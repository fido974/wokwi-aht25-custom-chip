// Wokwi Custom Chip - For information and examples see:
// https://link.wokwi.com/custom-chips-alpha
//
// SPDX-License-Identifier: MIT
// Copyright (C) 2022 Uri Shaked / wokwi.com

#include "wokwi-api.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

const int ADDRESS = 0x38; // I2C address for AHT25

typedef struct {
  pin_t pin_int;
  float temperature;
  float humidity;
  bool initialized;
  bool measuring;
  uint8_t read_buffer[7];
  uint8_t read_index;
  uint8_t status;
  uint8_t write_buffer[3];
  int write_index;
} chip_state_t;

static bool on_i2c_connect(void *user_data, uint32_t address, bool connect);
static uint8_t on_i2c_read(void *user_data);
static bool on_i2c_write(void *user_data, uint8_t data);
static void on_i2c_disconnect(void *user_data);

void chip_init() {
  chip_state_t *chip = malloc(sizeof(chip_state_t));

  chip->pin_int = pin_init("INT", INPUT);
  //chip->temperature = 20.0; // 20°C
  //chip->humidity = 65.0; // 65%
  // Version aleatoire des valeurs entre 18.0 and 25.0°C et 60.0 and 75.0%
  chip->temperature = 18.0 + (rand() % 800) / 100.0; // Random temperature between 18.0 and 25.0°C
  chip->humidity = 60.0 + (rand() % 1600) / 100.0;   // Random humidity between 60.0 and 75.0%

  chip->initialized = false;
  chip->measuring = false;
  chip->read_index = 0;
  chip->status = 0x18; // Default status indicating calibration is on and not busy
  chip->write_index = 0;

  const i2c_config_t i2c_config = {
    .user_data = chip,
    .address = ADDRESS,
    .scl = pin_init("SCL", INPUT),
    .sda = pin_init("SDA", INPUT),
    .connect = on_i2c_connect,
    .read = on_i2c_read,
    .write = on_i2c_write,
    .disconnect = on_i2c_disconnect, // Optional
  };
  i2c_init(&i2c_config);

  // The following message will appear in the browser's DevTools console:
  printf("AHT25 sensor initialized!\n");
}

bool on_i2c_connect(void *user_data, uint32_t address, bool connect) {
  chip_state_t *chip = user_data;
  if (chip->measuring) {
    // Reset pointer for a new transaction to deliver full measurement data
    chip->read_index = 0;
  }
  return (address == ADDRESS); /* Ack only if the address matches */
}

uint8_t on_i2c_read(void *user_data) {
  chip_state_t *chip = user_data;

  printf("on_i2c_read: chip->measuring = %s\n", chip->measuring ? "true" : "false");

  if (chip->measuring) {
    if (chip->read_index < 7) {
      uint8_t data = chip->read_buffer[chip->read_index++];
      printf("Reading byte %d: 0x%02X\n", chip->read_index - 1, data);
      // Print chip->read_buffer[] content
      printf("Read buffer content: ");
      for (int i = 0; i < 7; i++) {
        printf("%02X ", chip->read_buffer[i]);
      }
      printf("\n");
      return data;
    } else {
      chip->measuring = false;
      chip->read_index = 0;
      chip->status &= ~0x80;  // Clear busy flag
      printf("Measurement complete\n");
      
      return 0; // arbitrary value, won't be read
    }
  } else {
    printf("Reading status: 0x%02X\n", chip->status);
    return chip->status; // Return status byte
  }
}

bool on_i2c_write(void *user_data, uint8_t data) {
  chip_state_t *chip = user_data;

  printf("Write byte: 0x%02X, index: %d\n", data, chip->write_index);
  chip->write_buffer[chip->write_index++] = data;

  // Debug: Print entire write buffer content after adding new byte
  printf("Write buffer content: ");
  for (int i = 0; i < chip->write_index; i++) {
    printf("0x%02X ", chip->write_buffer[i]);
  }
  printf("\n");

  if (chip->write_index == 1 && data == 0xBA) {
    // Soft reset command branch
    chip->initialized = false;
    chip->measuring = false;
    chip->status = 0x18; // Reset status
    chip->write_index = 0;
    printf("Soft reset command received\n");
  } else if (chip->write_index == 3 && chip->write_buffer[0] == 0xBE) {
    // Initialization command branch
    chip->initialized = true;
    chip->status = 0x18; // Set calibration bit
    chip->write_index = 0;
    printf("Initialization command complete\n");
  } else if (chip->write_index == 3 &&
             chip->write_buffer[0] == 0x00 &&
             chip->write_buffer[1] == 0xAC &&
             chip->write_buffer[2] == 0x33) {
    // Measurement command branch per datasheet
    chip->measuring = true;
    printf("Mesure...\n");
    chip->status |= 0x80;  // Set busy flag correctly
    chip->write_index = 0;
    
    // ...simulate measurement delay and compute raw values...
    uint32_t rawHumidity = (uint32_t)((chip->humidity * 1048576) / 100);
    uint32_t rawTemperature = (uint32_t)(((chip->temperature + 50) * 1048576) / 200);
    
    chip->status &= ~0x80;  // Clear busy flag before preparing read buffer
    chip->read_buffer[0] = chip->status;
    chip->read_buffer[1] = (rawHumidity >> 12) & 0xFF;
    chip->read_buffer[2] = (rawHumidity >> 4) & 0xFF;
    chip->read_buffer[3] = ((rawHumidity & 0x0F) << 4) | ((rawTemperature >> 16) & 0x0F);
    chip->read_buffer[4] = (rawTemperature >> 8) & 0xFF;
    chip->read_buffer[5] = rawTemperature & 0xFF;
    uint8_t crc = 0xFF;
    for (int i = 0; i < 6; i++) {
      crc ^= chip->read_buffer[i];
      for (int j = 0; j < 8; j++) {
        if (crc & 0x80) { crc = (crc << 1) ^ 0x31; }
        else { crc <<= 1; }
      }
    }
    chip->read_buffer[6] = crc;
    printf("Measurement command complete, data ready\n");
  } else if (chip->write_index == 3) {
    // At write_index == 3 and command not recognized:
    printf("Command sequence received: ");
    for (int i = 0; i < 3; i++) {
      printf("0x%02X ", chip->write_buffer[i]);
    }
    printf("\nWarning: Unrecognized command sequence. Resetting write buffer.\n");
    chip->write_index = 0;
  }
  
  printf("on_i2c_write: chip->measuring = %s\n", chip->measuring ? "true" : "false");
  return true; // Ack
}

void on_i2c_disconnect(void *user_data) {
  // Do nothing
}
