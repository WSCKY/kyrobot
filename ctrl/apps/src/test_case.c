#include "apps.h"

#include <stdio.h>
#include "ringbuffer.h"
#include <string.h>
#include "cpu_utils.h"
#include "ff_gen_drv.h"

static const char *TAG = "TEST";

void test_fatfs(void);
//void test_w25qxx(void);
void scan_i2c_dev(void);
void check_ist83xx(void);
void list_file_fatfs(void);
void write_file(void);
void print_buff_test(void);

void test_case_task(void const *argument)
{
  (void) argument;
  ky_info(TAG, "test case task start.");
  osDelay(500);

//  scan_i2c_dev();
//  check_ist83xx();
//  test_w25qxx();
  test_fatfs();
//  list_file_fatfs();
//  write_file();
//  print_buff_test();

  ky_info(TAG, "test done.");

  vTaskDelete(NULL);
}

const uint8_t magdata[56] = {
0x2e, 0x6d, 0x63, 0x64, 0x0a, 0x02, 0xe4, 0xbc, 0x4a, 0xc8, 0xc1, 0x62, 0x79, 0x35, 0xc1, 0xf2,
0x2c, 0xbd, 0x41, 0x57, 0x8a, 0x6e, 0x3f, 0x9b, 0x93, 0x77, 0x3f, 0x2c, 0x67, 0x6e, 0x3f, 0xc2,
0x70, 0x71, 0x1a, 0xc0, 0x99, 0x9a, 0x3c, 0xc1, 0x42, 0x0c, 0xa0, 0x41, 0x19, 0x1f, 0x78, 0x3f,
0xc1, 0x9a, 0x7f, 0x3f, 0xf1, 0xad, 0x71, 0x3f,
};

void print_buff_test(void)
{
  ky_info(TAG, "log_buffer_hex:");
  log_buffer_hex("magdata", magdata, 56, LOG_INFO);
  ky_info(TAG, "log_buffer_char:");
  log_buffer_char("magdata", magdata, 56, LOG_INFO);
  ky_info(TAG, "log_buffer_hexdump:");
  log_buffer_hexdump("magdata", magdata, 56, LOG_INFO);
}

void write_file(void)
{
  FRESULT ret;
  uint32_t bytes_rw;
  FIL *file;
  uint8_t rtext[56];
  file = kmm_alloc(sizeof(FIL));
  if(file == NULL) {
    ky_err(TAG, "no enough memory.");
    return;
  }
  ky_info(TAG, "open file mag.dat");
  ret = f_open(file, "mag.dat", FA_CREATE_ALWAYS | FA_WRITE);
  if(ret != FR_OK) {
    ky_err(TAG, "failed to open/create file. %d", ret);
    goto exit;
  }

  ky_info(TAG, "write data to file.");
  ret = f_write(file, magdata, 56, (void *)&bytes_rw);
  if(bytes_rw == 0 || ret != FR_OK) {
    ky_err(TAG, "file write error. %d", ret);
    goto exit;
  }

  ky_info(TAG, "close file.");
  ret = f_close(file);
  if(ret != FR_OK) {
    ky_err(TAG, "failed to close file.");
    goto exit;
  }

  ky_info(TAG, "open file again");
  ret = f_open(file, "mag.dat", FA_READ);
  if(ret != FR_OK) {
    ky_err(TAG, "failed to open/read file. %d", ret);
    goto exit;
  }

  ky_info(TAG, "read data from file.");
  ret = f_read(file, rtext, sizeof(rtext), (UINT*)&bytes_rw);
  if(bytes_rw == 0 || ret != FR_OK) {
    ky_err(TAG, "file read error. %d", ret);
    goto exit;
  }

//  ky_info(TAG, "data in file: %s", rtext);
  log_buffer_hex("data", rtext, 56, LOG_VERBOSE);

  ky_info(TAG, "close file.");
  ret = f_close(file);
  if(ret != FR_OK) {
    ky_err(TAG, "failed to close file.");
  }

exit:
  kmm_free(file);
  return;
}

const char test_str[] = "FATFS TEST: Hello kyChu!";

static void list_dir(const TCHAR *path)
{
  FRESULT ret;
  DIR *dir;
  FILINFO *fno;
  dir = kmm_alloc(sizeof(DIR));
  fno = kmm_alloc(sizeof(FILINFO));
  if(dir == NULL || fno == NULL) {
    ky_err(TAG, "no memory for test.");
    goto exit;
  }
  ret = f_opendir(dir, path);
  if(ret != FR_OK) {
    ky_err(TAG, "open dir 0:/ failed.");
    goto exit;
  }

  do {
    ret = f_readdir(dir, fno);
    if(ret != FR_OK) {
      ky_err(TAG, "read %s failed.", path);
      break;
    } else if(fno->fname[0] == 0) {
      ky_info(TAG, "%s read done.", path);
      break;
    } else {
      ky_info(TAG, "%s: name: %s, size: %ld, altname: %s.", path, fno->fname, fno->fsize, fno->altname);
    }
  } while(1);

  ret = f_closedir(dir);
  if(ret != FR_OK) {
    ky_err(TAG, "close directory failed.");
    goto exit;
  }

  exit:
    kmm_free(dir);
    kmm_free(fno);
}

void list_file_fatfs(void)
{
  FRESULT ret;

  ret = f_mkdir("0:/calib");
  ky_info(TAG, "mkdir ret -%d.", ret);
  ret = f_mkdir("0:/firmware");
  ky_info(TAG, "mkdir ret -%d.", ret);
  ret = f_mkdir("0:/start");
  ky_info(TAG, "mkdir ret -%d.", ret);
  ret = f_mkdir("0:/calib/acc");
  ky_info(TAG, "mkdir ret -%d.", ret);
  ret = f_mkdir("0:/calib/gyr");
  ky_info(TAG, "mkdir ret -%d.", ret);
  ret = f_mkdir("0:/calib/mag");
  ky_info(TAG, "mkdir ret -%d.", ret);

  list_dir("0:/");
  list_dir("0:/calib");
  list_dir("0:/start");

  return;
}

void test_fatfs(void)
{
  FRESULT ret;
  uint32_t bytes_rw;
  uint8_t rtext[32];

  FIL *file;
  file = kmm_alloc(sizeof(FIL));
  if(file == NULL) {
    ky_err(TAG, "no enough memory.");
    return;
  }

  ky_info(TAG, "open file HELLO.txt");
  ret = f_open(file, "HELLO.txt", FA_CREATE_ALWAYS | FA_WRITE);
  if(ret != FR_OK) {
    ky_err(TAG, "failed to open/create file. %d", ret);
    goto exit;
  }

  ky_info(TAG, "write data(%s) to file.", test_str);
  ret = f_write(file, test_str, sizeof(test_str), (void *)&bytes_rw);
  if(bytes_rw == 0 || ret != FR_OK) {
    ky_err(TAG, "file write error. %d", ret);
    goto exit;
  }

  ky_info(TAG, "close file.");
  ret = f_close(file);
  if(ret != FR_OK) {
    ky_err(TAG, "failed to close file.");
    goto exit;
  }

  ky_info(TAG, "open file again");
  ret = f_open(file, "HELLO.txt", FA_READ);
  if(ret != FR_OK) {
    ky_err(TAG, "failed to open/read file. %d", ret);
    goto exit;
  }

  ky_info(TAG, "read data from file.");
  ret = f_read(file, rtext, sizeof(rtext), (UINT*)&bytes_rw);
  if(bytes_rw == 0 || ret != FR_OK) {
    ky_err(TAG, "file read error. %d", ret);
    goto exit;
  }

  ky_info(TAG, "data in file: %s", rtext);

  ky_info(TAG, "close file.");
  ret = f_close(file);
  if(ret != FR_OK) {
    ky_err(TAG, "failed to close file.");
  }

exit:
  kmm_free(file);
  return;
}

//void test_w25qxx(void)
//{
//  uint8_t id[3];
//  uint16_t wbytes = 1000;
//  uint32_t waddr = 3456;
//  uint8_t *wcache, *rcache;
//  if(w25qxx_init() != status_ok) {
//    ky_err(TAG, "failed to initialize w25qxx.");
//    return;
//  }
//  if(w25qxx_read_id(id) != status_ok) {
//    ky_err(TAG, "read w25qxx's id failed.");
//    return;
//  }
//  ky_info(TAG, "w25qxx device id: 0x%x, 0x%x, 0x%x", id[0], id[1], id[2]);
////  switch(id) {
////  case W25Q16: ky_info("w25q16 detected.\n"); break;
////  case W25Q32: ky_info("w25q32 detected.\n"); break;
////  case W25Q64: ky_info("w25q64 detected.\n"); break;
////  case W25Q128: ky_info("w25q128 detected.\n"); break;
////  default: ky_err("unknown device.\n"); return;
////  }
////  w25qxx_erase_chip();
//  if(w25qxx_write_unprotect() != status_ok) {
//    ky_err(TAG, "unprotect w25qxx fail.");
//    return;
//  }
//  ky_info(TAG, "flash read & write test.");
//  ky_info(TAG, "write %d bytes to 0x%lx.", wbytes, waddr);
//  wcache = kmm_alloc(wbytes);
//  rcache = kmm_alloc(wbytes);
//  if(wcache == NULL || rcache == NULL) {
//    ky_err(TAG, "no memory for test.");
//    return;
//  }
//  memset(rcache, 0x00, wbytes);
//  for(int i = 0; i < wbytes; i ++) {
//    wcache[i] = i + 0x63;
//  }
//  log_buffer_hex("wcache", wcache, wbytes, LOG_VERBOSE);
//
//  ky_info(TAG, "write start ...");
//  if(w25qxx_write_bytes(wcache, waddr, wbytes) != status_ok) {
//    ky_err(TAG, "write failed.");
//    kmm_free(wcache);
//    kmm_free(rcache);
//    return;
//  }
//  osDelay(100);
//  ky_info(TAG, "read start ...");
//  if(w25qxx_read_bytes(rcache, waddr, wbytes) != status_ok) {
//    ky_err(TAG, "read failed.");
//    kmm_free(wcache);
//    kmm_free(rcache);
//    return;
//  }
//  log_buffer_hex("rcache", rcache, wbytes, LOG_VERBOSE);
//
////  w25qxx_erase_sector(0);
////
////  osDelay(100);
////  ky_info("read start ...\n");
////  if(w25qxx_read_bytes(rcache, waddr, wbytes) != status_ok) {
////    ky_err("read failed.\n");
////    kmm_free(wcache);
////    kmm_free(rcache);
////    return;
////  }
////  log_buffer_hex("rcache", rcache, wbytes, LOG_VERBOSE);
//
//  ky_info(TAG, "compare data ...");
//  int i = 0;
//  do {
//    if(rcache[i] != wcache[i]) {
//      break;
//    }
//    i ++;
//  } while(i < wbytes);
//
//  if(i < wbytes) {
//    ky_err(TAG, "w25qxx test failed.-%d", i);
//  } else {
//    ky_info(TAG, "w25qxx test success.");
//  }
//  kmm_free(wcache);
//  kmm_free(rcache);
//
////  for(;;) {
////	  osDelay(100);
////	  w25qxx_erase_sector(0);
////  }
//}

void check_ist83xx(void)
{
  int cnt = 0;
  i16_3d_t *ist_raw_a = kmm_alloc(sizeof(i16_3d_t));
  i16_3d_t *ist_raw_b = kmm_alloc(sizeof(i16_3d_t));
  ist83xx_dev_t *ist8310_a = kmm_alloc(sizeof(ist83xx_dev_t));
  ist83xx_dev_t *ist8310_b = kmm_alloc(sizeof(ist83xx_dev_t));
  if((ist8310_a == NULL) || (ist8310_b == NULL)) {
    ky_err(TAG, "ist8310 memory alloc failed. EXIT!");
    return;
  }

  ist8310_a->dev_addr = 0x1A;
  ist8310_b->dev_addr = 0x1C;
  ist8310_a->io_init = ist8310_b->io_init = magif_init;
  ist8310_a->io_ready = ist8310_b->io_ready = magif_check_ready;
  ist8310_a->read_reg = ist8310_b->read_reg = magif_read_mem_dma;
  ist8310_a->write_reg = ist8310_b->write_reg = magif_write_mem_dma;

  ky_info(TAG, "init ist8310 a.");
  if(ist83xx_init(ist8310_a) != status_ok) {
    ky_err(TAG, "ist8310 a init failed.");
  }
  ky_info(TAG, "init ist8310 b.");
  if(ist83xx_init(ist8310_b) != status_ok) {
    ky_err(TAG, "ist8310 b init failed.");
  }

  for(;;) {
    osDelay(5);
    ist83xx_read_data(ist8310_a, ist_raw_a);
    ist83xx_read_data(ist8310_b, ist_raw_b);
    cnt ++;
    if(cnt >= 200) {
      cnt = 0;
      ky_info(TAG, "%3d, %3d, %3d;  %3d, %3d, %3d", ist_raw_a->x, ist_raw_a->y, ist_raw_a->z, ist_raw_b->x, ist_raw_b->y, ist_raw_b->z);
    }
  }
}

void scan_i2c_dev(void)
{
  uint8_t devaddr = 0x00;
  uint8_t reg_val = 0x00;
  if(magif_init() != status_ok) {
    ky_err(TAG, "mag if init failed. EXIT!");
    return;
  }

  osDelay(100);

  ky_info(TAG, "scanning i2c device ...");
  for(;;) {
    reg_val = 0x00;
    magif_read_mem_dma(devaddr, 0x00, &reg_val, 1);
    osDelay(10); // wait for transfer done.
    if((devaddr & 0x1F) == 0x00)
      log_write("\n\t");
    if(reg_val != 0x00)
      log_write("%02x  ", devaddr);
    else
      log_write("00  ");
    devaddr += 2;
    if(devaddr == 0x00) break;
  }
  ky_info(TAG, "\n\nscan i2c device done.");
}
