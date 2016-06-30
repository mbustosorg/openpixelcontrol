/* Copyright 2013 Ka-Ping Yee

Licensed under the Apache License, Version 2.0 (the "License"); you may not
use this file except in compliance with the License.  You may obtain a copy
of the License at: http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied.  See the License for the
specific language governing permissions and limitations under the License. */

#include "spi.h"
#include "opc.h"
#include <plog/Log.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <unistd.h>
#include <math.h>

static int spi_fd = -1;
static u8 spi_data_tx[((1 << 16) / 3) * 4 + 5];
static u32 spi_speed_hz = SPI_DEFAULT_SPEED_HZ;

static u8 gamma_table_red[256];
static u8 gamma_table_green[256];
static u8 gamma_table_blue[256];

const char* logFileName = "logs/tcl_server.log";
static int frameCount = 0;
#define FRAME_COUNT_INTERVAL_OUT (36000)

void tcl_put_pixels(int fd, u8 spi_data_tx[], u16 count, pixel* pixels) {
  int i;
  pixel* p;
  u8* d;
  u8 flag;
  u8 r, g, b;
  
  d = spi_data_tx;
  *d++ = 0;
  *d++ = 0;
  *d++ = 0;
  *d++ = 0;
  for (i = 0, p = pixels; i < count; i++, p++) {
    r = gamma_table_red[p->r];
    g = gamma_table_green[p->g];
    b = gamma_table_blue[p->b];
    flag = (r & 0xc0) >> 6 | (g & 0xc0) >> 4 | (b & 0xc0) >> 2;
    *d++ = ~flag;
    *d++ = r;
    *d++ = g;
    *d++ = b;
  }
  spi_write(fd, spi_data_tx, d - spi_data_tx);
}

void set_gamma(double gamma_red, double gamma_green, double gamma_blue) {
  int i;
  
  for (i = 0; i < 256; i++) {
    gamma_table_red[i] = (uint8_t)(pow(i / 255.0,gamma_red) * 255.0 + 0.5);
    gamma_table_green[i] = (uint8_t)(pow(i / 255.0,gamma_green) * 255.0 + 0.5);
    gamma_table_blue[i] = (uint8_t)(pow(i / 255.0,gamma_blue) * 255.0 + 0.5);
  }
}

void handler(u8 address, u16 count, pixel* pixels) {
  frameCount++;
  if (frameCount % FRAME_COUNT_INTERVAL_OUT == 0) {
    LOG_INFO << "Frames output: " << frameCount;
    frameCount = 0;
  }
  tcl_put_pixels(spi_fd, spi_data_tx, count, pixels);
}


int main(int argc, char** argv) {

  static plog::RollingFileAppender<plog::CsvFormatter> fileAppender(logFileName, 1000000, 3);
  static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
  plog::init(plog::info, &fileAppender).addAppender(&consoleAppender);

  LOG_INFO << "Logging to -> " << logFileName;

  u16 port = OPC_DEFAULT_PORT;
  pixel diagnostic_pixel;
  time_t t;

  //get_speed_and_port(&spi_speed_hz, &port, argc, argv);

  char spiDev[] = "/dev/spidev1.0";
  spi_fd = init_spidev(spiDev, spi_speed_hz);
  if (spi_fd < 0) {
    return 1;
  }

  /* set gamma correction */
  int opt = 0;
  float gammaValue = 1.0;
  while ((opt = getopt(argc, argv, "g:")) != -1) {
    switch (opt) {
    case 'g':
      gammaValue = atof(optarg);
      break;
    }
  }
  LOG_INFO << "Gamma correction factor: " << gammaValue;
  set_gamma(gammaValue, gammaValue, gammaValue);
  
  LOG_INFO << "SPI speed: " << spi_speed_hz*1e-6 << " MHz, ready...";
  opc_source s = opc_new_source(port);
  while (s >= 0) {
    u8 timeout = opc_receive(s, handler, 10000);
    if (timeout == 0) {
      LOG_INFO << "Timeout, no clients";
    }
  }

  t = time(NULL);
  diagnostic_pixel.r = (t % 3 == 0) ? 64 : 0;
  diagnostic_pixel.g = (t % 3 == 1) ? 64 : 0;
  diagnostic_pixel.b = (t % 3 == 2) ? 64 : 0;
  tcl_put_pixels(spi_fd, spi_data_tx, 1, &diagnostic_pixel);
}
