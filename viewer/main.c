#include <err.h>
#include <pthread.h>
#include <raylib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <vsync/queue/bounded_spsc.h>
#define TITLE "Cheap and Dirty Logic Analyzer with Teensy"
#define WINDOW_TITLE "Chadilat"
#define MAX_DATA 10000
#define QUEUE_SIZE 1024
#define HISTORY_MAX (1024 * 1024)

#define BOOL_COUNT 4
#define SCREEN_WIDTH 600
#define SCREEN_HEIGHT 300
#define MARGIN 50
#define MARGIN_SHIFT 20
#define SPACING ((SCREEN_HEIGHT - 2 * MARGIN) / (1 + BOOL_COUNT))
#define PLOT_END (SCREEN_WIDTH - MARGIN + MARGIN_SHIFT)
#define PLOT_START (MARGIN_SHIFT + MARGIN)
#define PLOT_WIDTH (PLOT_END - PLOT_START)

struct data_point {
  uint8_t counter;
  uint32_t timestamp;
  bool values[BOOL_COUNT];
  uint8_t value;
  struct data_point *next;
  struct data_point *prev;
};

bounded_spsc_t queue;

struct {
  struct data_point *start;
  struct data_point *end;
  uint64_t count;
} history;

static void history_append(struct data_point *pt) {
  pt->next = NULL;
  pt->prev = history.end;
  if (history.end)
    history.end->next = pt;
  history.end = pt;
  if (!history.start)
    history.start = pt;

  history.count++;
}

// Simulated data input
void generate_data() {
  for (int i = 0; i < 100; i++) {
    struct data_point *pt = malloc(sizeof(struct data_point));
    pt->timestamp = i;
    pt->counter = i;
    for (int j = 0; j < BOOL_COUNT; j++) {
      pt->values[j] = i % 2;
    }
    while (bounded_spsc_enq(&queue, pt) != QUEUE_BOUNDED_OK)
      ;
  }
}

static void *loader(void *arg) {
  const char *fname = (const char *)arg;
  FILE *fp = stdin;
  if (fname)
    fp = fopen(fname, "r");
  if (!fp)
    err(1, "could not open input");

  uint32_t counter;
  uint32_t timestamp;
  uint32_t pinb;
  while (3 == fscanf(fp, "%d %d %d\n", &counter, &timestamp, &pinb)) {
    struct data_point *pt = malloc(sizeof(struct data_point));
    pt->counter = (uint8_t)counter;
    pt->timestamp = timestamp;
    for (int j = 0; j < BOOL_COUNT; j++) {
      pt->values[j] = (pinb >> j) & 0x1;
    }
    pt->value = pinb;
    while (bounded_spsc_enq(&queue, pt) != QUEUE_BOUNDED_OK)
      ;
  }
  // generate_data();

  if (fname)
    fclose(fp);
  return NULL;
}

void run() {
  srand(time(0));
  InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, WINDOW_TITLE);
  SetTargetFPS(120);

  double tick_width = 1;
  unsigned int offset = 0;
  bool paused = false;

  struct data_point *cur = NULL;
  while (!WindowShouldClose()) {

    // new data point?
    struct data_point *pt = NULL;
    while (bounded_spsc_deq(&queue, (void **)&pt) == QUEUE_BOUNDED_OK) {
      history_append(pt);
    }

    // what is the current data point?
    if (cur == NULL) {
      if (!history.end)
        continue;
      cur = history.end;
    }

    // Input handling
    if (IsKeyPressed(KEY_Q)) {
      break;
    } else if (IsKeyPressed(KEY_SPACE)) {
      paused = !paused;
      if (!paused)
        cur = history.end;
    } else if (IsKeyDown(KEY_N)) {
      tick_width -= tick_width > 1 ? 1 : 0;
      if (tick_width <= 1)
        tick_width -= 0.05;
    } else if (IsKeyDown(KEY_M)) {
      tick_width += 1;
    } else if (IsKeyDown(KEY_L)) {
      cur = history.end;
    } else if (IsKeyDown(KEY_LEFT)) {
      if (!paused)
        paused = true;
      if (cur) {
        uint32_t now = cur->timestamp;
        while (cur->prev && now - cur->prev->timestamp <= tick_width)
          cur = cur->prev;
      }
    } else if (IsKeyDown(KEY_RIGHT)) {
      if (!paused)
        paused = true;
      if (cur) {
        uint32_t now = cur->timestamp;
        while (cur->next && cur->next->timestamp - now <= tick_width)
          cur = cur->next;
      }
    } else if (!paused) {
      if (cur && cur->next)
        cur = cur->next;

      while (history.count > HISTORY_MAX) {
        printf("Dropping\n");
        struct data_point *pt = history.start;
        assert(pt);
        assert(pt->next);
        pt->next->prev = NULL;
        history.start = pt->next;
        free(pt);
        history.count--;
      }
    }

    pt = cur;
    uint32_t now = cur->timestamp + offset * tick_width;

    // Drawing
    BeginDrawing();
    ClearBackground(RAYWHITE);

    DrawText(TITLE, 10, 10, 20, DARKGRAY);

    if (paused)
      DrawText("PAUSED", PLOT_WIDTH - MARGIN, 10, 10, RED);

    DrawLine(PLOT_START + 0, MARGIN + SPACING * 2 / 3, PLOT_START + 0,
             SCREEN_HEIGHT - MARGIN - SPACING * 2 / 3, GRAY);

    DrawText("time (ms)", (PLOT_START + PLOT_WIDTH / 2 - 8),
             SCREEN_HEIGHT - MARGIN - SPACING * 2 / 3 + 25, 16, GRAY);

    for (int i = PLOT_START; i < PLOT_END; i += PLOT_WIDTH / 10) {
      DrawLine(i, MARGIN + SPACING * 2 / 3, i,
               SCREEN_HEIGHT - MARGIN - SPACING * 2 / 3, GRAY);
      // 4µs per tick
      double tick_factor = 4.0 / tick_width / 1000.0;
      DrawText(TextFormat("%.2f", (i - PLOT_START) * tick_factor), i,
               SCREEN_HEIGHT - MARGIN - SPACING * 2 / 3 + 3, 12, GRAY);
    }

    for (; pt; pt = pt->prev) {

      int x0 = PLOT_END - (now - pt->timestamp) / tick_width;
      if (x0 <= PLOT_START)
        x0 = PLOT_START;

      for (int j = 0; j < BOOL_COUNT; j++) {
        int ybase = MARGIN + ((1 + j) * SPACING);
        bool b0 = pt->values[j];
        int y0 = ybase + (b0 ? -10 : 10);
        DrawLine(PLOT_START, ybase, PLOT_END, ybase, LIGHTGRAY);
        size_t font_size = 18;
        DrawText(TextFormat("PIN %i", j), MARGIN / 2 - 10,
                 ybase - font_size / 2, font_size, BLACK);

        // DrawCircle(x0, y0, 3, BLUE);
        if (pt->next == NULL)
          continue;
        bool b1 = pt->next->values[j];
        int x1 = PLOT_END - (now - pt->next->timestamp) / tick_width;
        int y1 = ybase + (b1 ? -10 : 10);

        if (b0 == b1) {
          DrawLine(x0, y0, x1, y1, BLUE);
        } else {
          int xx = (x0 + x1) / 2;
          DrawLine(x0, y0, xx, y0, BLUE);
          DrawLine(xx, y0, xx, y1, BLUE);
          DrawLine(xx, y1, x1, y1, BLUE);
        }
      }

      if (x0 <= PLOT_START)
        break;
    }
    EndDrawing();
  }

  CloseWindow();
}

int main(int argc, char *argv[]) {

  bounded_spsc_init(&queue, malloc(sizeof(void *) * QUEUE_SIZE), QUEUE_SIZE);
  pthread_t th;
  pthread_create(&th, 0, loader, argc >= 2 ? argv[1] : NULL);
  run();
  pthread_join(th, 0);
  return 0;
}
