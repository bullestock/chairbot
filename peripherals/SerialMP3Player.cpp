/*
 * SerialMP3Player.h - Library for Serial MP3 Player board from Catalex (YX5300 chip)
 * Created by Salvador Rueda Pau, July 23, 2016.
 * License as GPL v3 (http://www.gnu.org/licenses/gpl.html)
 *
 */

#include "Arduino.h"
#include "SerialMP3Player.h"

// Uncomment next line if you are using an Arduino Mega.
//#define mp3 Serial3    // Connect the MP3 Serial Player to the Arduino MEGA Serial3 (14 TX3 -> RX, 15 RX3 -> TX)

SerialMP3Player::SerialMP3Player(){
    Serial3 = new SoftwareSerial(10, 11);
    _showDebugMessages = false;
}
SerialMP3Player::SerialMP3Player(int RX, int TX){
    Serial3 = new SoftwareSerial(RX, TX);
    _showDebugMessages = false;
}

void SerialMP3Player::showDebug(bool op){
    // showDebug (op) 0:OFF 1:ON
    _showDebugMessages = op;
}

void SerialMP3Player::begin(int bps){
    Serial3->begin(bps);
}

int SerialMP3Player::available(){
    return Serial3->available();
}

unsigned char SerialMP3Player::read(){
    return Serial3->read();
}

void SerialMP3Player::playNext(){
    sendCommand(CMD_NEXT);
}

void SerialMP3Player::playPrevious(){
    sendCommand(CMD_PREV);
}

void SerialMP3Player::volUp(){
    sendCommand(CMD_VOL_UP);
}

void SerialMP3Player::volDown(){
    sendCommand(CMD_VOL_DOWN);
}

void SerialMP3Player::setVol(byte v){
    // Set volumen (0-30)
    sendCommand(CMD_SET_VOL, v);
}

void SerialMP3Player::playSL(byte n){
    // Play single loop n file
    sendCommand(CMD_PLAY_SLOOP, n);
}

void SerialMP3Player::playSL(byte f, byte n){
    // Single loop play n file from f folder
    sendCommand(CMD_PLAY_SLOOP, f, n);
}

void SerialMP3Player::play(){
    sendCommand(CMD_PLAY);
}

void SerialMP3Player::pause(){
    sendCommand(CMD_PAUSE);
}

void SerialMP3Player::play(byte n){
    // n number of the file that must be played.
    // n possible values (1-255)
    sendCommand(CMD_PLAYN, n);
}

void SerialMP3Player::play(byte n, byte vol){
    // n number of the file that must be played

    sendCommand(CMD_PLAY_W_VOL, vol, n);
}

void SerialMP3Player::playF(byte f){
    // Play all files in the f folder

    sendCommand(CMD_FOLDER_CYCLE, f, 0);
}

void SerialMP3Player::stop(){
    sendCommand(CMD_STOP_PLAY);
}

void SerialMP3Player::qPlaying(){
    // Ask for the file is playing
    sendCommand(CMD_PLAYING_N);
}

void SerialMP3Player::qStatus(){
    // Ask for the status.
    sendCommand(CMD_QUERY_STATUS);
}

void SerialMP3Player::qVol(){
    // Ask for the volumen
    sendCommand(CMD_QUERY_VOLUME);
}

void SerialMP3Player::qFTracks(){    // !!! Nonsense answer
    // Ask for the number of tracks folders
    sendCommand(CMD_QUERY_FLDR_TRACKS);
}

void SerialMP3Player::qTTracks(){
    // Ask for the total of tracks
    sendCommand(CMD_QUERY_TOT_TRACKS);
}

void SerialMP3Player::qTFolders(){
    // Ask for the number of folders
    sendCommand(CMD_QUERY_FLDR_COUNT);
}

void SerialMP3Player::sleep(){
    // Send sleep command
    sendCommand(CMD_SLEEP_MODE);
}

void SerialMP3Player::wakeup(){
    // Send wake up command
    sendCommand(CMD_WAKE_UP);
}

void SerialMP3Player::reset(){
    // Send reset command
    sendCommand(CMD_RESET);
}



void SerialMP3Player::sendCommand(byte command){
    sendCommand(command, 0, 0);
}

void SerialMP3Player::sendCommand(byte command, byte dat2){
    sendCommand(command, 0, dat2);
}


void SerialMP3Player::sendCommand(byte command, byte dat1, byte dat2){
    byte Send_buf[8] = {0}; // Buffer for Send commands.
    String mp3send = "";

    // Command Structure 0x7E 0xFF 0x06 CMD FBACK DAT1 DAT2 0xEF

#ifndef NO_SERIALMP3_DELAY
    delay(20);
#endif

    Send_buf[0] = 0x7E;    // Start byte
    Send_buf[1] = 0xFF;    // Version
    Send_buf[2] = 0x06;    // Command length not including Start and End byte.
    Send_buf[3] = command; // Command
    Send_buf[4] = 0x01;    // Feedback 0x00 NO, 0x01 YES
    Send_buf[5] = dat1;    // DATA1 datah
    Send_buf[6] = dat2;    // DATA2 datal
    Send_buf[7] = 0xEF;    // End byte

    for(int i=0; i<8; i++)
    {
        Serial3->write(Send_buf[i]) ;
        mp3send+=sbyte2hex(Send_buf[i]);
    }
    if (_showDebugMessages){
        Serial.print("Sending: ");
        Serial.println(mp3send); // watch what are we sending
    }

#ifndef NO_SERIALMP3_DELAY
    delay(1000);
    // Wait between sending commands.
#endif
}


/********************************************************************************/
/*Function: sbyte2hex. Returns a byte data in HEX format.	                */
/*Parameter:- uint8_t b. Byte to convert to HEX.                                */
/*Return: String                                                                */


String SerialMP3Player::sbyte2hex(byte b)
{
    String shex;
    if (b < 16)
        shex += "0";
    shex += String(b, HEX);
    shex +=" ";
    return shex;
}


bool SerialMP3Player::getAnswer(uint8_t& status, uint16_t& value)
{
    if (!Serial3->available())
    {
        Serial.println("No data");
        return false;
    }
    String mp3recv = "";
    // FS (7E)
    uint8_t b = Serial3->read();
    mp3recv += sbyte2hex(b);
    if (b != 0x7E)
    {
        Serial.println("No FS");
        return false;
    }
    // Version (FF)
    b = Serial3->read();
    mp3recv += sbyte2hex(b);
    if (b != 0xFF)
    {
        Serial.println("No version");
        return false;
    }
    // Length (06)
    b = Serial3->read();
    mp3recv += sbyte2hex(b);
    if (b != 6)
    {
        Serial.println("No length");
        return 0;
    }
    // Status
    status = Serial3->read();
    mp3recv += sbyte2hex(status);
    // Feedback
    b = Serial3->read(); // Feedback
    mp3recv += sbyte2hex(b);
    b = Serial3->read(); // Data high
    mp3recv += sbyte2hex(b);
    value = 256 * b;
    b = Serial3->read(); // Data low
    mp3recv += sbyte2hex(b);
    value += b;
    b = Serial3->read(); // Checksum high
    mp3recv += sbyte2hex(b);
    b = Serial3->read(); // Checksum low
    mp3recv += sbyte2hex(b);
    b = Serial3->read();
    mp3recv += sbyte2hex(b);
    if (b != 0xEF)
    {
        Serial.println("No FE");
        return false;
    }
    if (_showDebugMessages)
    {
        Serial.print("Received: ");
        Serial.println(mp3recv);
    }
    return true;
}
