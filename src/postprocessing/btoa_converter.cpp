#include "time_tsc.h"
#include "cloud_profiler.h"
#include "handlers/id_handler.h"
#include "globals.h"
#include "handlers/buffer_id_handler.h"
#include "handlers.h"

#include <string>
#include <dirent.h>
#include <iostream>
#include <cstring>
#include <fstream>
#include <squash.h>
#include <string.h>

void convert(bool decomp);
void convertGivenFiles(int argc, char ** argv);
DIR * openDirectory();
int closeDirectory(DIR * dir);
void convertAllLogFiles(DIR * dir, bool decomp);
void convertLogFile(std::string absolute_path, bool decomp);
void convertLogFileToAscii(std::string absolute_path);
void convertBinToASCII(std::string absolute_path, std::size_t idx);
void decompress(std::string absolute_path, std::size_t found, const char * codec);

int main(int argc, char * argv[]) {
  if (argc == 1) {
    convert(true);
    convert(false);
  } else {
    convertGivenFiles(argc, argv);
  }

  return 0;
}


// Convert log files
// decomp = true, convert from compressed data to decompressed one.
// decomp = false, convert from binary data to ascii one.

void convert(bool decomp) {
  // Open Directory
  DIR * dir = openDirectory();
  if (dir == NULL) {
    std::cout << "Cannot Open Directory!" << std::endl;
    exit(1);
  }

  // Convert binary log files
  convertAllLogFiles(dir, decomp);

  // Close Directory
  int ret = closeDirectory(dir);
  if (ret == -1) {
    std::cout << "Cannot Close Directory!" << std::endl;
    exit(1);
  }
}

// Open directory that contains cloud profiler log files.

DIR * openDirectory() {
  return opendir(path_to_output);
}

// Close directory that contains cloud profiler log files.

int closeDirectory(DIR * dir) {
  return closedir(dir);
}

// Convert log files in given directories.
// decomp = true, convert from compressed data to decompressed one.
// decomp = false, convert from binary data to ascii one.

void convertAllLogFiles(DIR * dir, bool decomp) {
  // directory entry
  struct dirent * directory_entry;
  std::string format_str_binary, format_str_zstd, format_str_lzo1x;
  format_to_str(BINARY, &format_str_binary);
  format_to_str(ZSTD, &format_str_zstd);
  format_to_str(LZO1X, &format_str_lzo1x);

  while ((directory_entry = readdir(dir)) != NULL) {
    std::string filename = directory_entry->d_name;
    convertLogFile(path_to_output + ("/" + filename), decomp);
  }
}

// Convert the given log file.
// decomp = true, convert from compressed data to decompressed one.
// decomp = false, convert from binary data to ascii one.

void convertLogFile(std::string absolute_path, bool decomp) {
  std::string format_str_binary, format_str_zstd, format_str_lzo1x;
  format_to_str(BINARY, &format_str_binary);
  format_to_str(ZSTD, &format_str_zstd);
  format_to_str(LZO1X, &format_str_lzo1x);

  // The name of the log files written in binary contains ':b:'
  // Likewise, ':z:' means zstd codec, ':l:' means lzo1x codec.
  std::size_t found_b = absolute_path.find(":" + format_str_binary + ":");
  std::size_t found_z = absolute_path.find(":" + format_str_zstd + ":");
  std::size_t found_l = absolute_path.find(":" + format_str_lzo1x + ":");

  // If the return value of find is std::string::npos,
  // then it means there is no matching.
  bool found_b_flag = (found_b != std::string::npos);
  bool found_z_flag = (found_z != std::string::npos);
  bool found_l_flag = (found_l != std::string::npos);

  if (decomp) {
    // Decompress data
    if (found_z_flag || found_l_flag) {
      std::cout << "Decompressing " << absolute_path << ".." << std::endl;
      decompress(absolute_path, (found_z_flag ? found_z : found_l),
        (found_z_flag ? format_str_zstd.c_str() : format_str_lzo1x.c_str()));
    }
  } else {
    // Convert binary to ascii
    if (found_b_flag) {
      std::cout << "Converting " << absolute_path << ".." << std::endl;
      convertBinToASCII(absolute_path, found_b);
    }
  }
}

// Convert the given log file to ascii.

void convertLogFileToAscii(std::string absolute_path) {
  std::string format_str_binary, format_str_zstd, format_str_lzo1x;
  format_to_str(BINARY, &format_str_binary);
  format_to_str(ZSTD, &format_str_zstd);
  format_to_str(LZO1X, &format_str_lzo1x);

  // The name of the log files written in binary contains ':b:'
  // Likewise, ':z:' means zstd codec, ':l:' means lzo1x codec.
  std::size_t found_b = absolute_path.find(":" + format_str_binary + ":");
  std::size_t found_z = absolute_path.find(":" + format_str_zstd + ":");
  std::size_t found_l = absolute_path.find(":" + format_str_lzo1x + ":");

  // If the return value of find is std::string::npos,
  // then it means there is no matching.
  bool found_b_flag = (found_b != std::string::npos);
  bool found_z_flag = (found_z != std::string::npos);
  bool found_l_flag = (found_l != std::string::npos);

  // Decompress data
  if (found_z_flag || found_l_flag) {
    std::cout << "Decompressing " << absolute_path << ".." << std::endl;
    decompress(absolute_path, (found_z_flag ? found_z : found_l),
      (found_z_flag ? format_str_zstd.c_str() : format_str_lzo1x.c_str()));

    std::cout << "Converting decompressed data" << ".." << std::endl;
    convertBinToASCII(absolute_path, (found_z_flag ? found_z : found_l));
  } else if (found_b_flag) {
    std::cout << "Converting " << absolute_path << ".." << std::endl;
    convertBinToASCII(absolute_path, found_b);
  }
}

// Convert the binary log files into ASCII log files

void convertBinToASCII(std::string absolute_path, std::size_t idx) {
  std::string format_str_ascii, format_str_binary;
  format_to_str(ASCII, &format_str_ascii);
  format_to_str(BINARY, &format_str_binary);

  // Open binary log files to read
  std::ifstream log_to_read;
  log_to_read.open(absolute_path.replace(idx + 1, 1, format_str_binary),
                                      std::ios::in | std::ios::binary);

  // Create another file to store converted logs
  std::ofstream log_to_write;
  log_to_write.open(absolute_path.replace(idx + 1, 1, format_str_ascii),
                                      std::ios::out | std::ios::trunc);

  int64_t tuple;
  struct timespec ts;

  while (true) {
    // Read logs written in binary and write in ASCII form.
    readInBinary(&log_to_read, &tuple, &ts);

    if (log_to_read.eof())
      break;

    writeInAscii(&log_to_write, tuple, &ts);
  }

  log_to_read.close();
  log_to_write.close();
}

// Decompress compressed log file

void decompress(std::string absolute_path, std::size_t idx, const char * codec) {
  std::string format_str_binary, format_str_zstd;
  format_to_str(BINARY, &format_str_binary);
  format_to_str(ZSTD, &format_str_zstd);

  // Open compressed log files to read
  std::ifstream log_to_read;
  log_to_read.open(absolute_path.replace(idx + 1, 1, codec),
                                      std::ios::in | std::ios::binary);

  // Create another file to store decompressed logs
  std::ofstream log_to_write;
  log_to_write.open(absolute_path.replace(idx + 1, 1, format_str_binary),
                          std::ios::out | std::ios::trunc | std::ios::binary);

  char * in = new char[BUFFER_BLOCK_SIZE * 40];
  char * out = new char[BUFFER_BLOCK_SIZE * 40];

  int block_size;
  size_t size;
  // Read compressed data, decompress, and write to another file.
  while (true) {
    // Read the size of block
    log_to_read.read((char *) &block_size, sizeof(block_size));

    // Read one block
    log_to_read.read((char *) in, block_size);

    if (log_to_read.eof())
      break;

    // Decompress data
    size = BUFFER_BLOCK_SIZE * 40;
    SquashCodec * c = (!strcmp(codec, format_str_zstd.c_str()) ?
                        squash_get_codec("zstd") : squash_get_codec("lzo1x"));
    SquashStatus result = squash_codec_decompress(c, &size,
                            (uint8_t *) out, block_size, (uint8_t *) in, NULL);

    // Check whether compression succeeds
    if (result != SQUASH_OK) {
      std::cerr << "Unable to decompress data: "
                              << squash_status_to_string(result) << std::endl;
    }

    log_to_write.write(out, size);
  }

  // Close files
  log_to_read.close();
  log_to_write.close();

  delete[] in;
  delete[] out;
}

void convertGivenFiles(int argc, char ** argv) {
  for (int i = 1; i < argc; i++) {
    convertLogFileToAscii(argv[i]);
  }
}