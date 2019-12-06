#include <SPI.h>
#include <SD.h>

void WriteLine(char* filename, String line) {
  File myxFile;
  myxFile = SD.open(filename, FILE_WRITE);
  myxFile.print(line);
  myxFile.close();
}

String ReadLine(char* filename, int x = 1) {
  File myxFile;
  String line = "";
  myxFile = SD.open(filename);
  for (int i = 1; i < x; i++) {
    while (myxFile.available()) {
      if (myxFile.read() == '\n' ) {
        break;
      }
    }
  }
  while (myxFile.available()) {
    char c = myxFile.read();
    line += c;
    if (c == '\n' ) {
      break;
    }
  }
  myxFile.close();
  return line;
}

int NumberOfLogs(char* filename) {
  File myxFile;
  int i = 0;
  if (myxFile = SD.open(filename)) {
    while (myxFile.available()) {
      if (myxFile.read() == '\n' ) {
        i++;
      }
    }
    myxFile.close();
  }
  return i;
}

void CopyFile(char* filename1, char* filename2) {
  SD.remove(filename2);
  int x = NumberOfLogs(filename1);
  for (int i = 1; i <= x; i++) {
    WriteLine(filename2, ReadLine(filename1 , i));
  }
}

void RemoveOldLogs (char* filename, int trgr, int x = 10) {
  SD.remove("tempfile.txt");
  int y = NumberOfLogs(filename);
  for (int i = (y - trgr + x + 1) ; i <= y; i++) {
    WriteLine("tempfile.txt", ReadLine(filename, i) );
  }
  CopyFile("tempfile.txt", filename);
  SD.remove("tempfile.txt");
}

void PrintFile(char* filename) {
  File myxFile;
  myxFile = SD.open(filename);
  if (myxFile) {
    while (myxFile.available()) {
      Serial.write(myxFile.read());
    }
    myxFile.close();
  } else {
    Serial.println("Can't show content, error opening file");
  }
}
