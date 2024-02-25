#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <gpiod.h>
#include <unistd.h> 

#define GPIO_CHIP_NAME "gpiochip0"  // Replace if using a different chip
#define GPIO_LINE_OFFSET 0  // Replace if using a different line

char result[3000] = {'1','0','1','0','1','0','1','0','1','0','1','1','1','1','1','1','1','1','1','1'};
int counter = 20;

void chartobin(char c) {
  int i;
  for (i = 7; i >= 0; i--) {
    result[counter] = (c & (1 << i)) ? '1' : '0';
    counter++;
  }
}

void int2bin(unsigned integer, int n) {
  for (int i = 0; i < n; i++) {
    result[counter] = (integer & (int)1 << (n - i - 1)) ? '1' : '0';
    result[36] = '\0';
    counter++;
  }
}

int main() {
  struct timeval tval_before, tval_after, tval_result;
  struct gpiod_chip *chip;
  struct gpiod_line *line;
  int ret, length, pos = 0;

  // --- libgpiod Initialization ---
  chip = gpiod_chip_open_by_name(GPIO_CHIP_NAME);
  if (!chip) {
    perror("gpiod_chip_open_by_name");
    return -1;
  }

  line = gpiod_chip_get_line(chip, GPIO_LINE_OFFSET);
  if (!line) {
    perror("gpiod_chip_get_line");
    gpiod_chip_close(chip);
    return -1;
  }

  // Request the GPIO line (output mode)
  struct gpiod_line_request_config config = {
      .consumer = "Sending data",
      .request_type = GPIOD_LINE_REQUEST_DIRECTION_OUTPUT,
      .flags = 0,
  };
  ret = gpiod_line_request(line, &config, 0);
  if (ret < 0) {
    perror("gpiod_line_request");
    gpiod_line_put(line); // Release the line if request failed
    gpiod_chip_close(chip);
    return -1;
  }


  // --- Message Input and Encoding ---
  char msg[3000];
  int len, k;

  printf("\nEnter the Message: ");
  scanf("%[^'\n']", msg); 

  len = strlen(msg);
  int2bin(len * 8, 16); 
  printf("Frame Header (Synchro and Textlength = %s\n", result);

  for (k = 0; k < len; k++) {
    chartobin(msg[k]);
  }

  // --- Transmission with Timing ---
  length = strlen(result);
  gettimeofday(&tval_before, NULL);

  while (pos != length) {
    gettimeofday(&tval_after, NULL);
    timersub(&tval_after, &tval_before, &tval_result);
    double time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);

    while (time_elapsed < 0.001) { // Adjust the delay if needed
      gettimeofday(&tval_after, NULL);
      timersub(&tval_after, &tval_before, &tval_result);
      time_elapsed = (double)tval_result.tv_sec + ((double)tval_result.tv_usec / 1000000.0f);
    }

    gettimeofday(&tval_before, NULL); 

    if (result[pos] == '1') {
      gpiod_line_set_value(line, 1);  // Set GPIO high
    } else {
      gpiod_line_set_value(line, 0);  // Set GPIO low
    }
    pos++;
  }

  // --- Cleanup ---
  gpiod_line_release(line);
  gpiod_chip_close(chip);
  return 0;
}
