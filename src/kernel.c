
/*
*
*	The Kernel
*
*/

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "kernel.h"
#include "hal/hal.h"
#include "string.h"

#include "drivers/stdio/emb-stdio.h"			// Needed for printf
#include "drivers/sdcard/SDCard.h"

void kernel_init(void);
void input_output_init(void);
void sys_info( uint8_t* );
void sd_card_fs_demo();
void check_command(char*);
void echo(char* input);
void ls(char* input);
void cat(char* input);
void cd(char* input);
void cur_working_dir();

char curr_dir[16] = "\\*";



/*
 *		Kernel's entry point
**/
void main(uint32_t r0, uint32_t r1, uint32_t atags){

  //Init
  kernel_init();
  input_output_init();

  sd_card_fs_demo();   //<<-- Uncomment this to show File System/SD Card demo

  //Welcome Msg Video
  hal_io_video_puts( "\n\r\n\rWelcome to MiniOS Pi Zero", 3, VIDEO_COLOR_GREEN );
  hal_io_serial_puts( SerialA, "\n\r\n\rWelcome to MiniOS Pi Zero" );

  uint8_t c;
  char inbuf[1024];
  size_t incount = 0;

  cur_working_dir();

  while (1){

    c = hal_io_serial_getc( SerialA );
    if (c == '\r'){
      inbuf[incount] = '\0';
      check_command(inbuf);
      memset(inbuf, 0, sizeof inbuf);

      incount = 0;
      cur_working_dir();

    } else {
      inbuf[incount] = c;
      incount++;
      printf_serial( "%c", c );
      printf_video( "%c", c );  //<<--- We also have printfs
    }

  }
  /////////////////////////////////////////////////////
  // uint32_t ret_val = ((int*)(void)(&buffer[0]))();
  // printf_serial(ret_val);
  // printf_video(ret_val);
  /////////////////////////////////////////////////////

}

/*
* Check what command the user entered
*/
void check_command(char* input){
  char command[64];
  char* arg;
  strcpy(command, strtok(input, " "));
  arg = &input[strlen(command)+1];

  if (strcmp("echo", command) == 0){
    echo(arg);
  } else if (strcmp("ls", command) == 0){
    ls(arg);
  } else if (strcmp("cat", command) == 0){
    cat(arg);
  } else if (strcmp(input, "sysinfo") == 0) {
    sys_info(SYSTEM_INFO);
  } else if (strcmp("cd", command) == 0) {
    cd(arg);
  }
}

/*
* echo back user input
*/
void echo(char* input){
    hal_io_serial_puts( SerialA, "\n\r " );
    hal_io_serial_puts( SerialA, input);
    hal_io_serial_puts( SerialA, "\n\r$ " );

    hal_io_video_puts("\n\r", 2, VIDEO_COLOR_WHITE);
    hal_io_video_puts(input, 2, VIDEO_COLOR_WHITE);
}

/*
* list curr dir or given dir
*/
void ls(char* input){
  //for now ignore input and list root dir
  char dirName[1024];
  //hal_io_video_puts("\n\r", 2, VIDEO_COLOR_WHITE);
  //strcpy(dirName, "\\*");
  HANDLE fh;
	FIND_DATA find;
	char* month[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	fh = sdFindFirstFile(curr_dir, &find);							// Find first file
	do {
		if (find.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY){
      printf_serial("%s <DIR>\n", find.cFileName);
      printf_video("%s <DIR>\n", find.cFileName);
    }
		else {
      printf_serial("%c%c%c%c%c%c%c%c.%c%c%c Size: %9lu bytes, %2d/%s/%4d, LFN: %s\n",
			find.cAlternateFileName[0], find.cAlternateFileName[1],
			find.cAlternateFileName[2], find.cAlternateFileName[3],
			find.cAlternateFileName[4], find.cAlternateFileName[5],
			find.cAlternateFileName[6], find.cAlternateFileName[7],
			find.cAlternateFileName[8], find.cAlternateFileName[9],
			find.cAlternateFileName[10],
			(unsigned long)find.nFileSizeLow,
			find.CreateDT.tm_mday, month[find.CreateDT.tm_mon],
			find.CreateDT.tm_year + 1900,
			find.cFileName);

      printf_video("Size: %9lu bytes, Name: %s\n\r",
			(unsigned long)find.nFileSizeLow,
      find.cFileName);
    }										// Display each entry
	} while (sdFindNextFile(fh, &find) != 0);						// Loop finding next file
	sdFindClose(fh);												// Close the serach handle


}

/*
* print contents of file to screen
*/
void cat(char* input){
  char buf[101];

  // create a temp path to create the absolute path of the file name
  char temp_path[24] = "";
  strcat(temp_path, curr_dir);
  for (int i = 0; i < 24; i++) {
    if (temp_path[i] == '*') {
      temp_path[i] = 0;
    }
  }
  strcat(temp_path, input);

  HANDLE fHandle = sdCreateFile(temp_path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (fHandle != 0) {
    uint32_t bytesRead;
    int readres;
    do {
      if (((readres = sdReadFileRetInt(fHandle, &buf[0], 100, &bytesRead, 0)) == 0))  {
        buf[bytesRead] = '\0';  ///insert null char
        printf_serial("%s", &buf[0]);
        hal_io_video_puts(buf, 2, VIDEO_COLOR_WHITE);
      }
      else if (readres == 1 ){
        buf[bytesRead] = '\0';  ///insert null char
        printf_serial("%s", &buf[0]);
        hal_io_video_puts(buf, 2, VIDEO_COLOR_WHITE);
        printf_serial("EOF" );
      }
      else if (readres == 2 ){
        printf_serial("read fail 2" );
      }
      else if (readres == 3 ){
        printf_serial("read fail 3" );
      }
      else if (readres == 4 ){
        printf_serial("read fail 4" );
      }
      else if (readres == 5 ){
        printf_serial("read fail 5" );
      }
      else if (readres == 6 ){
        printf_serial("read fail 6" );
      }
    } while (bytesRead > 0);



    // Close the file
    sdCloseHandle(fHandle);
  }
}

 /*
 * Change current directory
 */
void cd(char* input) {
  HANDLE fh;
  FIND_DATA find;

  if (strcmp(input, "..") == 0) {
    // Check if not in root folder
    if (strlen(curr_dir) != 2) {
      int numBack = 0;

      // Removes first dir from the right
      for (int i = 16; i > 0; i--) {
        if (curr_dir[i] == '*') {           // *
          curr_dir[i] = 0;
        } else if (curr_dir[i] == '\\') {   // back slash
          if (numBack == 1) {               // second back slash
            break;
          }
          numBack++;
          curr_dir[i] = 0;
        } else {                            // letter
          curr_dir[i] = 0;
        }
      }

      // Adds a * at the end of root dir
      if (strlen(curr_dir) == 1) {
        curr_dir[1] = '*';
      }

      // Adds a * at the end of path
      int zero = 0;
      for (int i = 0; i > 16; i++) {
        if (zero == 1) {
          break;
        } else if (curr_dir[i] == 0) {
          curr_dir[i] = '*';
          zero++;
        }
      }
    } else {
      // Already in root folder
      printf_serial("\n\rCurrently in root folder\n\r");
      hal_io_video_puts("\n\rCurrently in root folder\n\r", 2, VIDEO_COLOR_WHITE);
    }
  } else {
    fh = sdFindFirstFile(curr_dir, &find);
    do {
      // Check if input matches a file name
      if (strcmp(find.cFileName, input) == 0) {
        // Make sure file is a dir
        if (find.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
          int input_len = strlen(input);

          // Append dir to end of curr_dir
          for (int i = 0; i < 16; i++) {
            if (curr_dir[i] == '*' && curr_dir[i+1] == 0) {
              int j;

              for (j = 0; j < input_len; j++) {
                curr_dir[i+j] = input[j];
              }

              curr_dir[i+j] = '\\';
              curr_dir[i+j+1] = '*';

              break;
            }
          }
        }
      }
	  } while (sdFindNextFile(fh, &find) != 0);
  }

  sdFindClose(fh);
}

/*
* Initializes the kernel
*/
void kernel_init(void){

  hal_io_init();
  //console_init();
  //system_calls_init();
  //scheduler_init();
  //faults_init();

}

/*
* Initializes All IO
*/
void input_output_init(void){
  uint32_t video_init_res = HAL_FAILED;

#ifdef VIDEO_PRESENT
  video_init_res = hal_io_video_init();
#endif

#ifdef SERIAL_PRESENT
  hal_io_serial_init();
#endif
//NOTE: PAST THIS POINT WE CAN USE printf
//      (printf needs both serial and video inits to be called first)

  if ( video_init_res == HAL_SUCCESS )
    sys_info( "Video Initialized\n\r" );
  else
    sys_info( "Video Init Failed [x]\n\r" );

    sys_info( "Serial Initialized\n\r" );
}

void sys_info( uint8_t* msg ){
  printf_video( msg );
  printf_serial( msg );
}

/////////////////////////////////////////////////////////////////
////////////////    D E M O    C O D E    ///////////////////////
/////////////////////////////////////////////////////////////////

char buffer[500];
void DisplayDirectory(const char*);


void sd_card_fs_demo(){
  printf_serial("\n\n");
  sdInitCard (&printf_video, &printf_serial, true);

  /* Display root directory */
  printf_serial("\n\nDirectory (/): \n");
  DisplayDirectory("\\*.*");

  printf_serial("\n");
  printf_serial("Opening Alice.txt \n");

  HANDLE fHandle = sdCreateFile("Alice.txt", GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
  if (fHandle != 0) {
    uint32_t bytesRead;

    if ((sdReadFile(fHandle, &buffer[0], 500, &bytesRead, 0) == true))  {
        buffer[bytesRead-1] = '\0';  ///insert null char
        printf_serial("File Contents: %s", &buffer[0]);
    }
    else{
      printf_serial("Failed to read" );
    }

    // Close the file
    sdCloseHandle(fHandle);

  }


}

void DisplayDirectory(const char* dirName) {
	HANDLE fh;
	FIND_DATA find;
	char* month[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
	fh = sdFindFirstFile(dirName, &find);							// Find first file
	do {
		if (find.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY)
			printf_serial("%s <DIR>\n", find.cFileName);
		else printf_serial("%c%c%c%c%c%c%c%c.%c%c%c Size: %9lu bytes, %2d/%s/%4d, LFN: %s\n",
			find.cAlternateFileName[0], find.cAlternateFileName[1],
			find.cAlternateFileName[2], find.cAlternateFileName[3],
			find.cAlternateFileName[4], find.cAlternateFileName[5],
			find.cAlternateFileName[6], find.cAlternateFileName[7],
			find.cAlternateFileName[8], find.cAlternateFileName[9],
			find.cAlternateFileName[10],
			(unsigned long)find.nFileSizeLow,
			find.CreateDT.tm_mday, month[find.CreateDT.tm_mon],
			find.CreateDT.tm_year + 1900,
			find.cFileName);										// Display each entry
	} while (sdFindNextFile(fh, &find) != 0);						// Loop finding next file
	sdFindClose(fh);												// Close the serach handle
}

/*
 * Prints the current working directory
 */
 void cur_working_dir() {
   printf_serial("\n\r$ %s\n\r", curr_dir);
   printf_video("\n\r$ %s\n\r", curr_dir);
 }
