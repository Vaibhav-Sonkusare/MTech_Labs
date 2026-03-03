/*
 * Protocol Comparison Simulator
 *
 * Compares the Blast Protocol against standard reliable transfer protocols:
 * - Stop-and-Wait
 * - Go-Back-N
 * - Selective Repeat
 *
 * Tracks both time complexity (RTTs) and space complexity (memory usage).
 */

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============================================================================
 * SIMULATION PARAMETERS
 * ============================================================================
 */

#define FILE_SIZE (1 * 1024 * 1024) /* 1 MB */
#define PACKET_SIZE 1024            /* bytes per packet */
#define RTT_MS 10                   /* Round trip time in ms */
#define WINDOW_SIZE 16              /* Window size for windowed protocols */
#define PACKETS_PER_BLAST 16        /* Packets per blast */
#define RECORDS_PER_PACKET 16       /* Records per packet */

#define NUM_RUNS 100 /* Runs per configuration */

static const double LOSS_RATES[] = {0.0, 0.10, 0.30, 0.50};
#define NUM_LOSS_RATES 4

#define TOTAL_PACKETS ((FILE_SIZE + PACKET_SIZE - 1) / PACKET_SIZE)

/* ============================================================================
 * MEMORY TRACKING
 * ============================================================================
 */

static size_t g_current_mem = 0;
static size_t g_peak_mem = 0;

static void mem_reset(void) {
  g_current_mem = 0;
  g_peak_mem = 0;
}

static void *mem_alloc(size_t size) {
  void *ptr = malloc(size);
  if (ptr) {
    g_current_mem += size;
    if (g_current_mem > g_peak_mem) {
      g_peak_mem = g_current_mem;
    }
  }
  return ptr;
}

static void mem_free(void *ptr, size_t size) {
  if (ptr) {
    free(ptr);
    if (g_current_mem >= size) {
      g_current_mem -= size;
    }
  }
}

static size_t mem_get_peak(void) { return g_peak_mem; }

/* ============================================================================
 * SIMULATION RESULT
 * ============================================================================
 */

typedef struct {
  char protocol_name[64];
  double loss_rate;
  uint64_t total_packets_sent;
  uint64_t total_acks_sent;
  uint64_t total_rtts;
  double estimated_time_ms;
  double efficiency;
  size_t peak_memory_bytes;
} SimResult;

/* ============================================================================
 * UTILITY FUNCTIONS
 * ============================================================================
 */

static bool packet_lost(double loss_rate) {
  return ((double)rand() / RAND_MAX) < loss_rate;
}

/* ============================================================================
 * STOP-AND-WAIT PROTOCOL
 * ============================================================================
 */

static SimResult simulate_stop_and_wait(double loss_rate) {
  mem_reset();

  uint64_t packets_sent = 0;
  uint64_t acks_sent = 0;
  uint64_t rtts = 0;
  uint32_t packets_delivered = 0;

  /* Memory: just tracking current packet (minimal) */
  uint8_t *pkt_buffer = mem_alloc(PACKET_SIZE);

  while (packets_delivered < TOTAL_PACKETS) {
    packets_sent++;
    rtts++;

    if (packet_lost(loss_rate))
      continue;

    acks_sent++;

    if (packet_lost(loss_rate))
      continue;

    packets_delivered++;
  }

  mem_free(pkt_buffer, PACKET_SIZE);

  double estimated_time = rtts * RTT_MS;
  uint64_t total_data_sent = packets_sent * PACKET_SIZE + acks_sent * 20;
  uint64_t useful_data = TOTAL_PACKETS * PACKET_SIZE;
  double efficiency = (double)useful_data / total_data_sent;

  SimResult result = {0};
  snprintf(result.protocol_name, sizeof(result.protocol_name), "Stop-and-Wait");
  result.loss_rate = loss_rate;
  result.total_packets_sent = packets_sent;
  result.total_acks_sent = acks_sent;
  result.total_rtts = rtts;
  result.estimated_time_ms = estimated_time;
  result.efficiency = efficiency;
  result.peak_memory_bytes = mem_get_peak();

  return result;
}

/* ============================================================================
 * GO-BACK-N PROTOCOL
 * ============================================================================
 */

static SimResult simulate_go_back_n(double loss_rate) {
  mem_reset();

  uint64_t packets_sent = 0;
  uint64_t acks_sent = 0;
  uint64_t rtts = 0;
  uint32_t base = 0;

  /* Memory: window buffer for N packets */
  size_t window_buf_size = WINDOW_SIZE * PACKET_SIZE;
  uint8_t *window_buffer = mem_alloc(window_buf_size);

  while (base < TOTAL_PACKETS) {
    uint32_t window_end = base + WINDOW_SIZE;
    if (window_end > TOTAL_PACKETS)
      window_end = TOTAL_PACKETS;
    uint32_t packets_in_window = window_end - base;

    packets_sent += packets_in_window;
    rtts++;

    /* Find first lost packet */
    uint32_t first_loss = packets_in_window; /* No loss by default */
    for (uint32_t i = 0; i < packets_in_window; i++) {
      if (packet_lost(loss_rate)) {
        first_loss = i;
        break;
      }
    }

    if (first_loss < packets_in_window) {
      acks_sent += first_loss;
      base += first_loss;
    } else {
      acks_sent++;
      if (packet_lost(loss_rate))
        continue;
      base = window_end;
    }
  }

  mem_free(window_buffer, window_buf_size);

  double estimated_time = rtts * RTT_MS;
  uint64_t total_data_sent = packets_sent * PACKET_SIZE + acks_sent * 20;
  uint64_t useful_data = TOTAL_PACKETS * PACKET_SIZE;
  double efficiency = (double)useful_data / total_data_sent;

  SimResult result = {0};
  snprintf(result.protocol_name, sizeof(result.protocol_name),
           "Go-Back-N (N=%d)", WINDOW_SIZE);
  result.loss_rate = loss_rate;
  result.total_packets_sent = packets_sent;
  result.total_acks_sent = acks_sent;
  result.total_rtts = rtts;
  result.estimated_time_ms = estimated_time;
  result.efficiency = efficiency;
  result.peak_memory_bytes = mem_get_peak();

  return result;
}

/* ============================================================================
 * SELECTIVE REPEAT PROTOCOL
 * ============================================================================
 */

static SimResult simulate_selective_repeat(double loss_rate) {
  mem_reset();

  uint64_t packets_sent = 0;
  uint64_t acks_sent = 0;
  uint64_t rtts = 0;

  /* Memory: received bitmap + window buffer + per-packet timers */
  size_t bitmap_size = (TOTAL_PACKETS + 7) / 8;
  size_t window_buf_size = WINDOW_SIZE * PACKET_SIZE;
  size_t timer_size = WINDOW_SIZE * sizeof(uint32_t);

  uint8_t *received = mem_alloc(bitmap_size);
  uint8_t *window_buffer = mem_alloc(window_buf_size);
  uint32_t *timers = mem_alloc(timer_size);

  memset(received, 0, bitmap_size);

  uint32_t base = 0;

  while (base < TOTAL_PACKETS) {
    uint32_t window_end = base + WINDOW_SIZE;
    if (window_end > TOTAL_PACKETS)
      window_end = TOTAL_PACKETS;

    /* Find packets to send */
    uint32_t to_send_count = 0;
    for (uint32_t i = base; i < window_end; i++) {
      if (!(received[i / 8] & (1 << (i % 8)))) {
        to_send_count++;
      }
    }

    if (to_send_count == 0) {
      while (base < TOTAL_PACKETS && (received[base / 8] & (1 << (base % 8)))) {
        base++;
      }
      continue;
    }

    packets_sent += to_send_count;
    rtts++;

    /* Simulate delivery */
    for (uint32_t i = base; i < window_end; i++) {
      if (!(received[i / 8] & (1 << (i % 8)))) {
        if (!packet_lost(loss_rate)) {
          received[i / 8] |= (1 << (i % 8));
        }
      }
    }

    acks_sent++;
    if (packet_lost(loss_rate)) {
      /* ACK lost */
    }

    while (base < TOTAL_PACKETS && (received[base / 8] & (1 << (base % 8)))) {
      base++;
    }
  }

  mem_free(timers, timer_size);
  mem_free(window_buffer, window_buf_size);
  mem_free(received, bitmap_size);

  double estimated_time = rtts * RTT_MS;
  uint64_t total_data_sent = packets_sent * PACKET_SIZE + acks_sent * 50;
  uint64_t useful_data = TOTAL_PACKETS * PACKET_SIZE;
  double efficiency = (double)useful_data / total_data_sent;

  SimResult result = {0};
  snprintf(result.protocol_name, sizeof(result.protocol_name),
           "Selective Repeat (N=%d)", WINDOW_SIZE);
  result.loss_rate = loss_rate;
  result.total_packets_sent = packets_sent;
  result.total_acks_sent = acks_sent;
  result.total_rtts = rtts;
  result.estimated_time_ms = estimated_time;
  result.efficiency = efficiency;
  result.peak_memory_bytes = mem_get_peak();

  return result;
}

/* ============================================================================
 * BLAST PROTOCOL
 * ============================================================================
 */

static SimResult simulate_blast_protocol(double loss_rate) {
  mem_reset();

  uint32_t records_per_blast = PACKETS_PER_BLAST * RECORDS_PER_PACKET;
  uint32_t record_size = PACKET_SIZE / RECORDS_PER_PACKET;
  uint32_t total_records = (FILE_SIZE + record_size - 1) / record_size;
  uint32_t total_blasts =
      (total_records + records_per_blast - 1) / records_per_blast;

  uint64_t packets_sent = 0;
  uint64_t acks_sent = 0;
  uint64_t rtts = 0;

  /* Memory: blast buffer + received bitmap per blast */
  size_t blast_buf_size = records_per_blast * record_size;
  size_t bitmap_size = (PACKETS_PER_BLAST + 7) / 8;

  uint8_t *blast_buffer = mem_alloc(blast_buf_size);
  uint8_t *received = mem_alloc(bitmap_size);

  for (uint32_t blast_id = 0; blast_id < total_blasts; blast_id++) {
    uint32_t start_record = blast_id * records_per_blast;
    uint32_t end_record = start_record + records_per_blast;
    if (end_record > total_records)
      end_record = total_records;
    uint32_t records_in_blast = end_record - start_record;
    uint32_t packets_in_blast =
        (records_in_blast + RECORDS_PER_PACKET - 1) / RECORDS_PER_PACKET;

    memset(received, 0, bitmap_size);

    bool blast_complete = false;
    while (!blast_complete) {
      /* Count unacked packets */
      uint32_t to_send = 0;
      for (uint32_t i = 0; i < packets_in_blast; i++) {
        if (!(received[i / 8] & (1 << (i % 8)))) {
          to_send++;
        }
      }

      packets_sent += to_send;
      rtts++;

      /* Simulate delivery */
      for (uint32_t i = 0; i < packets_in_blast; i++) {
        if (!(received[i / 8] & (1 << (i % 8)))) {
          if (!packet_lost(loss_rate)) {
            received[i / 8] |= (1 << (i % 8));
          }
        }
      }

      /* IS_BLAST_OVER */
      packets_sent++;
      if (packet_lost(loss_rate))
        continue;

      /* REC_MISS */
      acks_sent++;
      if (packet_lost(loss_rate))
        continue;

      /* Check if all received */
      blast_complete = true;
      for (uint32_t i = 0; i < packets_in_blast; i++) {
        if (!(received[i / 8] & (1 << (i % 8)))) {
          blast_complete = false;
          break;
        }
      }
    }
  }

  mem_free(received, bitmap_size);
  mem_free(blast_buffer, blast_buf_size);

  double estimated_time = rtts * RTT_MS;
  uint32_t blast_packet_size = RECORDS_PER_PACKET * record_size;
  uint64_t total_data_sent = packets_sent * blast_packet_size + acks_sent * 50;
  uint64_t useful_data = FILE_SIZE;
  double efficiency = (double)useful_data / total_data_sent;

  SimResult result = {0};
  snprintf(result.protocol_name, sizeof(result.protocol_name),
           "Blast Protocol (%dx%d)", PACKETS_PER_BLAST, RECORDS_PER_PACKET);
  result.loss_rate = loss_rate;
  result.total_packets_sent = packets_sent;
  result.total_acks_sent = acks_sent;
  result.total_rtts = rtts;
  result.estimated_time_ms = estimated_time;
  result.efficiency = efficiency;
  result.peak_memory_bytes = mem_get_peak();

  return result;
}

/* ============================================================================
 * AVERAGING HELPER
 * ============================================================================
 */

static SimResult average_results(SimResult *results, int count) {
  SimResult avg = {0};
  if (count == 0)
    return avg;

  snprintf(avg.protocol_name, sizeof(avg.protocol_name), "%s",
           results[0].protocol_name);
  avg.loss_rate = results[0].loss_rate;

  for (int i = 0; i < count; i++) {
    avg.total_packets_sent += results[i].total_packets_sent;
    avg.total_acks_sent += results[i].total_acks_sent;
    avg.total_rtts += results[i].total_rtts;
    avg.estimated_time_ms += results[i].estimated_time_ms;
    avg.efficiency += results[i].efficiency;
    if (results[i].peak_memory_bytes > avg.peak_memory_bytes) {
      avg.peak_memory_bytes = results[i].peak_memory_bytes;
    }
  }

  avg.total_packets_sent /= count;
  avg.total_acks_sent /= count;
  avg.total_rtts /= count;
  avg.estimated_time_ms /= count;
  avg.efficiency /= count;

  return avg;
}

/* ============================================================================
 * PRINT RESULTS
 * ============================================================================
 */

static void print_header(void) {
  printf("\n");
  printf("====================================================================="
         "===========\n");
  printf("                    PROTOCOL COMPARISON SIMULATION RESULTS\n");
  printf("====================================================================="
         "===========\n");
  printf("File Size: %.1f MB | Packet Size: %d bytes | RTT: %d ms\n",
         (double)FILE_SIZE / (1024 * 1024), PACKET_SIZE, RTT_MS);
  printf("Total Packets (ideal): %u | Runs per config: %d\n", TOTAL_PACKETS,
         NUM_RUNS);
  printf("====================================================================="
         "===========\n");
}

static void print_results_for_loss_rate(SimResult *results, int count,
                                        double loss_rate) {
  printf("\n");
  printf("---------------------------------------------------------------------"
         "-----------\n");
  printf("  PACKET LOSS RATE: %.0f%%\n", loss_rate * 100);
  printf("---------------------------------------------------------------------"
         "-----------\n");
  printf("%-28s %12s %10s %8s %12s %10s %12s\n", "Protocol", "Pkts Sent",
         "ACKs", "RTTs", "Time(ms)", "Effic.", "Memory");
  printf("%-28s %12s %10s %8s %12s %10s %12s\n", "----------------------------",
         "------------", "----------", "--------", "------------", "----------",
         "------------");

  double min_time = 1e18;
  for (int i = 0; i < count; i++) {
    if (results[i].estimated_time_ms < min_time) {
      min_time = results[i].estimated_time_ms;
    }
  }

  for (int i = 0; i < count; i++) {
    printf("%-28s %12lu %10lu %8lu %12.0f %9.1f%% %10zuKB\n",
           results[i].protocol_name,
           (unsigned long)results[i].total_packets_sent,
           (unsigned long)results[i].total_acks_sent,
           (unsigned long)results[i].total_rtts, results[i].estimated_time_ms,
           results[i].efficiency * 100, results[i].peak_memory_bytes / 1024);
  }

  printf("\n  Relative Performance (time):\n");
  for (int i = 0; i < count; i++) {
    double rel = results[i].estimated_time_ms / min_time;
    int bar_len = (int)(rel * 10);
    if (bar_len > 40)
      bar_len = 40;

    printf("    %-28s ", results[i].protocol_name);
    for (int j = 0; j < bar_len; j++)
      printf("█");

    if (rel == 1.0) {
      printf(" ⭐ FASTEST\n");
    } else {
      printf(" %.2fx slower\n", rel);
    }
  }
}

static void print_memory_summary(SimResult *all_results, int total_count) {
  printf("\n");
  printf("====================================================================="
         "===========\n");
  printf("                           MEMORY USAGE SUMMARY\n");
  printf("====================================================================="
         "===========\n");
  printf("%-28s %15s %s\n", "Protocol", "Peak Memory", "Notes");
  printf("%-28s %15s %s\n", "----------------------------", "---------------",
         "----------------------------------------");

  /* Extract unique protocols */
  const char *printed[4] = {NULL};
  int printed_count = 0;

  for (int i = 0; i < total_count; i++) {
    bool already_printed = false;
    for (int j = 0; j < printed_count; j++) {
      if (printed[j] && strcmp(printed[j], all_results[i].protocol_name) == 0) {
        already_printed = true;
        break;
      }
    }
    if (already_printed)
      continue;

    printed[printed_count++] = all_results[i].protocol_name;

    const char *note = "";
    if (strstr(all_results[i].protocol_name, "Stop")) {
      note = "O(1) - single packet buffer";
    } else if (strstr(all_results[i].protocol_name, "Go-Back")) {
      note = "O(N) - window buffer";
    } else if (strstr(all_results[i].protocol_name, "Selective")) {
      note = "O(N + total_pkts) - window + bitmap";
    } else if (strstr(all_results[i].protocol_name, "Blast")) {
      note = "O(blast_size) - blast buffer + bitmap";
    }

    printf("%-28s %12zuKB   %s\n", all_results[i].protocol_name,
           all_results[i].peak_memory_bytes / 1024, note);
  }
}

static void print_analysis(void) {
  printf("\n");
  printf("====================================================================="
         "===========\n");
  printf("                              ANALYSIS SUMMARY\n");
  printf("====================================================================="
         "===========\n");
  printf("\n");
  printf("TIME COMPLEXITY (RTTs needed):\n");
  printf("  Stop-and-Wait:    O(n)           - 1 RTT per packet\n");
  printf("  Go-Back-N:        O(n/N)         - 1 RTT per window, but "
         "retransmits on loss\n");
  printf("  Selective Repeat: O(n/N)         - 1 RTT per window, selective "
         "retransmit\n");
  printf("  Blast Protocol:   O(blasts)      - 1 RTT per blast cycle\n");
  printf("\n");
  printf("SPACE COMPLEXITY:\n");
  printf("  Stop-and-Wait:    O(1)           - Single packet buffer\n");
  printf("  Go-Back-N:        O(N)           - Window of N packets\n");
  printf(
      "  Selective Repeat: O(N + n)       - Window buffer + received bitmap\n");
  printf("  Blast Protocol:   O(blast_size)  - Full blast buffer (~16KB)\n");
  printf("\n");
  printf("PROTOCOL SUITABILITY:\n");
  printf("  Protocol          Best For                         Weakness\n");
  printf("  "
         "---------------------------------------------------------------------"
         "------\n");
  printf("  Stop-and-Wait     Simple, low memory               Very slow\n");
  printf("  Go-Back-N         Moderate loss, simpler           Wastes "
         "bandwidth on loss\n");
  printf("  Selective Repeat  General purpose, efficient       Complex "
         "implementation\n");
  printf("  Blast Protocol    Bulk transfers, high throughput  Higher memory "
         "usage\n");
  printf("\n");
}

/* ============================================================================
 * MAIN
 * ============================================================================
 */

int main(void) {
  srand((unsigned int)time(NULL));

  printf("Running protocol comparison simulation (%d runs per config)...\n\n",
         NUM_RUNS);

  print_header();

  SimResult all_results[NUM_LOSS_RATES * 4];
  int result_count = 0;

  for (int lr = 0; lr < NUM_LOSS_RATES; lr++) {
    double loss_rate = LOSS_RATES[lr];

    SimResult saw_results[NUM_RUNS];
    SimResult gbn_results[NUM_RUNS];
    SimResult sr_results[NUM_RUNS];
    SimResult blast_results[NUM_RUNS];

    for (int run = 0; run < NUM_RUNS; run++) {
      saw_results[run] = simulate_stop_and_wait(loss_rate);
      gbn_results[run] = simulate_go_back_n(loss_rate);
      sr_results[run] = simulate_selective_repeat(loss_rate);
      blast_results[run] = simulate_blast_protocol(loss_rate);
    }

    SimResult avg_saw = average_results(saw_results, NUM_RUNS);
    SimResult avg_gbn = average_results(gbn_results, NUM_RUNS);
    SimResult avg_sr = average_results(sr_results, NUM_RUNS);
    SimResult avg_blast = average_results(blast_results, NUM_RUNS);

    SimResult loss_results[4] = {avg_saw, avg_gbn, avg_sr, avg_blast};
    print_results_for_loss_rate(loss_results, 4, loss_rate);

    all_results[result_count++] = avg_saw;
    all_results[result_count++] = avg_gbn;
    all_results[result_count++] = avg_sr;
    all_results[result_count++] = avg_blast;
  }

  print_memory_summary(all_results, result_count);
  print_analysis();

  return 0;
}
