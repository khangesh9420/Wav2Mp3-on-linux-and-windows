// wav2mp3 convert a wav music file to an mp3 encoded files with same name but change 
// extension, like music.wav -> music.mp3. 
#include <unistd.h>   //Posix for maintain compatbility between os
#include <semaphore.h>//sem_t use type use to provide semaphore operations
#include <dirent.h>   //Retrieve information about


#include "lame.h"    //MPEG audio layer III(MP3)encoder
#include <iostream>  //input output library
#include <fstream>   //Input/output stream class to operate on files
#include <string>
#include <vector>    //use contiguous storage locations
#include <algorithm> //collection of functions
#include <thread>    //sequence of instructions that can be executed
#include <mutex>     // to protect shared data from multiple threading

#define DEBUG       1         //0 prints nothing and 1 print when a file was converted
#define PCM_SIZE    8192
#define MP3_SIZE    8192
#define SAMPLE_RATE 44100


//Global variables
typedef std::vector<std::string> String_Vector;                             // to save file list
unsigned int                     Cores=std::thread::hardware_concurrency(); // number of cores
sem_t                            Task_Sem;                                  // one semaphore for each task
std::mutex                       Print_Mutex;                               // to print concurrently

//Prototypes
void Search_Files              ( const std::string& dir, String_Vector& files     );
void Convert_Wav_To_Mp3          ( const std::string& Dir, String_Vector& Wav_Files );
int  Test_Single_File_Wav_To_Mp3 ( std::string Wav_File_Name);

//Main
int main(int argc, char** argv)
{/*{{{*/
   if(argc < 2 ) {
#if DEBUG==1
      std::cout << "no input files" << std::endl;
#endif
      exit(1);
   }
   else {
#if DEBUG==1
      std::cout << "wavfiles working with " << Cores << " cores" << std::endl;
#endif
      std::string   Dir_Name=argv[1];              //expect the first parameter was the dir
      String_Vector Files;
      if(Dir_Name.back()!='/')
         Dir_Name+='/';                            //add the "/" if the user doesnt include 
      sem_init           ( &Task_Sem,0,Cores );    //one sem per core (dinamically allocate Cores sem)
      Search_Files     ( Dir_Name,Files    );    //put  files names in Files string vector
      Convert_Wav_To_Mp3 ( Dir_Name,Files    );    // for Multitasking 
   }
   return 0;
}
//functions
//Read dir and fill Files with all wav (case insensitive) files inside Dir
void Search_Files(const std::string& Dir, String_Vector& Files)
{ 
   DIR* Dirp = opendir(Dir.c_str()); // it will open the  file directory,return the pointer to a directory structure
   struct dirent* Dp;
   while ((Dp = readdir(Dirp)) != NULL) {
      std::string File_Name=Dp->d_name;                                                                  //get the file name under dir
      if(File_Name.length() > 4 ) {                                                                      //min 1.wav is 4 chars
         std::string File_Extension = File_Name.substr(File_Name.find_last_of(".") + 1);                 //find the dot from back
         std::transform(File_Extension.begin(),File_Extension.end(), File_Extension.begin(), ::tolower); //all to lower. It'll work on wav,WAV,WaV, etc.
         if(File_Extension=="wav")                                                                       //finded?
            Files.push_back(File_Name);                                                                  //add for proccess
      }
   }
   closedir(Dirp);
}
//dispatch one task per core to convert wavTOmp3 in detached mode, Thasts why  here semaphore is used 

void Convert_Wav_To_Mp3(const std::string& Dir, String_Vector& Wav_Files)/*{{{*/
{
   for(auto F: Wav_Files) {
      sem_wait(&Task_Sem);                                                    //stop here waiting sem_get for multiple threads i.e multiple audio files at a time like 3 or 4 filees 
      std::thread Thread=std::thread(Test_Single_File_Wav_To_Mp3,(Dir + F));  //ok, I've free core, atach to a task
      Thread.detach();                                                        //but if I detach the task from main, the for continue searching next free core, and the task runs on his way
   }
   //when all wav in his own task, wait for all sems being post but without overload the core
   //that runs main
   for ( int Pending=0;Pending<Cores;sem_getvalue(&Task_Sem,&Pending ))
      sleep(0.1);                                                             //if I don't wait, in the end I'll overload the core, doing nothing
}
//it's a C snippet from lame doc to read a file and convert to mp3 with rasonable settings
//I added a debug message when the file was ready or an error when something happend
int Test_Single_File_Wav_To_Mp3(std::string Wav_File_Name)
{
   int read, write;
   bool Ok=false;

   std::string Mp3_File_Name=Wav_File_Name.substr ( 0,Wav_File_Name.find_last_of(".")+1) + "mp3";

   FILE *pcm = fopen(Wav_File_Name.c_str(), "rb");
   FILE *mp3 = fopen(Mp3_File_Name.c_str(), "wb");
   if(pcm != NULL && mp3 != NULL) {
      Ok=true;                                  //just error check
      short int     pcm_buffer[ PCM_SIZE*2 ];
      unsigned char mp3_buffer[ MP3_SIZE   ];

      lame_t lame = lame_init();
      lame_set_in_samplerate ( lame, SAMPLE_RATE );
      lame_set_VBR           ( lame, vbr_default );
      lame_init_params       ( lame              );

      do {
         read = fread(pcm_buffer, 2*sizeof(short int), PCM_SIZE, pcm);
         if (read == 0)
            write = lame_encode_flush(lame, mp3_buffer, MP3_SIZE);
         else
            write = lame_encode_buffer_interleaved(lame, pcm_buffer, read, mp3_buffer, MP3_SIZE);
         fwrite(mp3_buffer, write, 1, mp3);
      } while (read != 0);

      lame_close ( lame );
      fclose     ( mp3  );
      fclose     ( pcm  );
   }
#if DEBUG==1
   if(Ok == true) {
      Print_Mutex.lock(); // lock mutex we used mutex here to acess multiple threads but only single resource at a time so thats why we used mutex here in the function of Test_Single_File_Wav_To_Mp3.
         std::cout<< Wav_File_Name << " -> " << Mp3_File_Name << std::endl; //debug
      Print_Mutex.unlock();// unlock mutex and ready for next task
   }
#endif
   sem_post(&Task_Sem); //free the semaphore and next file will come and to the process again.
   return 0;            //it's the same as pthread_exit(0)
}

