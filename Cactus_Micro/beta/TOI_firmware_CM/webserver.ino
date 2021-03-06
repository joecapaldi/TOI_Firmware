/*
 * This file is part of TOI_firmware.
 *
 * Copyright (C) 2015  D.Herrendoerfer
 *
 *   TOI_firmware is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   TOI_firmware is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with TOI_firmware.  If not, see <http://www.gnu.org/licenses/>.
 */

//#define SERV_DEBUG 1

#define strlenPGM(str) (sizeof(str)-1) 

int request_get( char* ip_buffer, int length, int ch_id)
{
  char *URL;
  URL = strstr(ip_buffer,"GET")+4;
  
  *(strstr(ip_buffer,"HTTP")-1) = 0;

#ifdef SERV_DEBUG  
  Serial.print(F("URL: "));
  Serial.println(URL);
  Serial.print(F("ID: "));
  Serial.println(ch_id);
  Serial.print(F("buff_len: "));
  Serial.println(length);
#endif

  if (!myAppHandleURL(URL, ch_id)) {
    /* Request was handeled/caught by myApp code
       don't handle any further. */
    return 0;
  }

  /* 
   * Built-in pages for configuration and test 
   */
  if (strcmp(URL,"/arduino.html") == 0 ){
    /* arduino page */
    page_arduino(ch_id, URL);
    return 0;
  }

  if (strstr(URL,"/reboot.html?") == URL) {
    /* reboot page */
    page_reboot(ch_id, URL);
    return 0;
  }

  if (strcmp(URL,"/config.html") == 0) {
    /* config page */
    page_config(ch_id, URL);
    return 0;
  }

  if (strstr(URL,"/c2ee.html?") == URL) {
    /* connfig save page */
    page_c2ee(ch_id, URL);
    return 0;
  }

  if (strstr(URL,"/c2ct.html?") == URL) {
    /* time save page */
    page_c2ct(ch_id, URL);
    return 0;
  }

  if (strcmp(URL,"/cdate.html") == 0) {
    /* show time page */
    page_cdate(ch_id, URL);
    return 0;
  }

  if (strstr(URL,"/cntp.html?") == URL) {
    /* time save page */
    page_cntp(ch_id, URL);
    return 0;
  }

  response_404(ch_id);
  return 0;
}

int intLen(unsigned int x) {
    if(x>=10000) return 5;
    if(x>=1000) return 4;
    if(x>=100) return 3;
    if(x>=10) return 2;
    return 1;
}

prog_char header[] PROGMEM = "HTTP/1.1 200 OK\r\n" 
                        "Content-Type: text/html\r\n"
                        "Connection: close\r\n"
                        "Content-Length: ";

int response_head(int ch_id, int length)
{
  /*Important note: sizeof() is 1 larger than strlen() would be
    but when using PROGMEM sizeof is required                   */
  int len = strlenPGM(header) + intLen(length) + 4;

  if (send_ipdata_head(ch_id, NULL, len)) {
    close_channel(ch_id);
    return 1;
  }

  send_progmem_data((char*)header, strlenPGM(header));
  send_ipdata(length);
  send_ipdata("\r\n\r\n");
  
  return send_ipdata_fin();
}

prog_char stream_header[] PROGMEM = "HTTP/1.1 200 OK\r\n" 
                        "Content-Type: text/html\r\n"
                        "Connection: close\r\n\r\n";

int stream_head(int ch_id)
{
  /*Important note: sizeof() is 1 larger than strlen() would be
    but when using PROGMEM sizeof is required                   */
  if (send_ipdata_head(ch_id, NULL, strlenPGM(stream_header)))
    return 1;
  send_progmem_data((char*)stream_header, strlenPGM(stream_header));
  
  return send_ipdata_fin();
}

int stream_send(int ch_id, char* data)
{
  if (send_ipdata_head(ch_id,data, strlen(data)))
    return 1;
  return send_ipdata_fin();
}

int stream_send(int ch_id, char* data, int length)
{
  if (send_ipdata_head(ch_id,data, length))
    return 1;
  return send_ipdata_fin();
}

int response_send_simple(int ch_id, char* content, int length)
{
  if (response_head(ch_id,length))
    goto err_out;
    
  if (send_ipdata_head(ch_id, content, length))
    goto err_out;
    
  if (send_ipdata_fin())
    goto err_out;
  
  return 0;
  
  err_out:
  close_channel(ch_id);
  return 1;

}

int response_send_progmem(int ch_id, PGM_P content, int length)
{
  int xfer = 0;
  length--;
  if (response_head(ch_id,length))
    goto err_out;

  /* Send bigger things in chunks of 240 bytes,
   * the ESP may accept more, but sometimes it don't
   */
  while (length) {
    int sndlen = 240;
    if (length < 240)
      sndlen = length;
      
    if (send_ipdata_head(ch_id, NULL, sndlen))
      goto err_out;

    send_progmem_data(content+xfer, sndlen);
  
    if (send_ipdata_fin())
      goto err_out;

    length -= sndlen;
    xfer += 240;
  }

  return 0;
  
  err_out:
  close_channel(ch_id);
  return 1;
}

prog_char error_404[] PROGMEM = "HTTP/1.1 404 ERROR\r\n"
                        "Content-Type: text/html\r\n"
                        "Connection: close\r\n"
                        "Content-Length: 9\r\n\r\n"
                        "Error 404";

void response_404(int ch_id)
{
  if (send_ipdata_head(ch_id, NULL, strlenPGM(error_404)))
    goto err_out;

  send_progmem_data((char*)error_404, strlenPGM(error_404));

  if (send_ipdata_fin())
    goto err_out;

  return;
  
  err_out:
  close_channel(ch_id);
  return;
}

/*********************************************************************************************** 
*  arduino.html 
*/
prog_char content_arduino[] PROGMEM =  "<HTML><BODY>arduino.html<hr>Built-in pages:<p>"
                          "<a href=config.html>Configuration</a><br>"
                          "<a href=reboot.html?reboot>reboot</a><br>"
                          "<a href=reboot.html?shutdown>shutdown</a><br>"
                          "<a href=reboot.html?default>default settings</a>"
                          "</BODY></HTML>\r\n";
  
void page_arduino(int ch_id, char* URL) {
  response_send_progmem(ch_id, content_arduino, strlenPGM(content_arduino));
}

/*********************************************************************************************** 
*  reboot.html 
*/
prog_char content_reboot[] PROGMEM = "<HTML><BODY>Arduino reboot.html<hr>will shutdown/reboot to selected settings</BODY></HTML>\r\n";

void page_reboot(int ch_id, char* URL)
{
  if (response_send_progmem(ch_id, (char*)content_reboot, strlenPGM(content_reboot)))
    return;

  if (strstr(URL,"?default")) {
    invalidate_eeprom();
    defaultAP=1;
  } else {
    defaultAP=0;
  }
  
  if (strstr(URL,"?shutdown")) {
    reboot = 0;
  } else {
    reboot = 1;
  }
  
  shutdown=1;
  
  return;
}

/*********************************************************************************************** 
*  config.html 
*/
int eeprom_unlock = 0;
prog_char content_config[] PROGMEM = "<HTML><BODY>Arduino config.html<hr>"
                         "<h5>Wifi Setup<br>"
                         "<form action=\"c2ee.html\" method=\"get\">"
                         "SSID:<input name=\"ssid\" value=\"MySSID\"><br>"
                         "Password:<input name=\"pass\" value=\"MyPASSWORD\"><br>"
                         "<input type=\"checkbox\" name=\"ap\" value=\"on\">Run as AP<br>"
                         "<input type=\"checkbox\" name=\"enc\" value=\"on\">Run as AP with encryption"
                         "<br>AP on channel:<input name=\"chan\" value=\"10\" size=\"2\"><br>"
                         "<button>Save</button></form>"
                         "<hr><form action=\"c2ct.html\" method=\"get\">"
                         "Set time:<input name=\"ch\" value=\"12\" size=\"2\">"
                         ":<input name=\"cm\" value=\"0\" size=\"2\"><br>"
                         "Day(0-7):<input name=\"cd\" value=\"0\" size=\"2\"><br>"
                         "Timebase:<input name=\"cb\" value=\"1000000\" size=\"7\"><br>"
                         "<button>Set</button></form>"
                         "<hr><form action=\"cntp.html\" method=\"get\">"
                         "NTP client setup:<br><input type=\"checkbox\" name=\"ntp\" value=\"on\">Enable NTP<br>"
                         "NTP server IP:<input name=\"ip\" value=\"192.168.1.1\"><br>"
                         "Timezone: UTC+<input name=\"tz\" value=\"1\"><br>"
                         "<button>Set</button></form>"
                         "</BODY></HTML>\r\n";

void page_config(int ch_id, char* URL)
{
  eeprom_unlock=1;  
  response_send_progmem(ch_id, content_config, strlenPGM(content_config));
}

/*********************************************************************************************** 
*  c2ee.html 
*/
prog_char content_c2ee[] PROGMEM = "<HTML><BODY>SAVE<hr>OK<p><a href=reboot.html?reboot>reboot</a></BODY></HTML>\r\n";

void page_c2ee(int ch_id, char* URL)
{
  char* Params = strstr(URL,"?")+1;

#ifdef SERV_DEBUG  
  Serial.print(F("Params:"));
  Serial.println(Params);
#endif

  char* r_ssid=strstr(Params,"ssid=")+5;
  char* r_pass=strstr(Params,"pass=")+5;
  char* r_chan=strstr(Params,"chan=")+5;

    if (strstr(Params,"ap=on")) {
      e_AP=1;
    } else {
      e_AP=0;
    }

    if (strstr(Params,"enc=on")) {
      e_ENC=3;
    } else {
      e_ENC=0;
    }

  if (strstr(r_ssid,"&"))
    *(strstr(r_ssid,"&")) = 0;
  if (strstr(r_pass,"&"))
    *(strstr(r_pass,"&")) = 0;
  if (strstr(r_chan,"&"))
    *(strstr(r_chan,"&")) = 0;

  sscanf(r_chan, "%d", &e_CHAN );
  
#ifdef SERV_DEBUG  
  Serial.println(F("Cred:"));
  Serial.print(F("SSID:"));
  Serial.println(r_ssid);
  Serial.print(F("PASS:"));
  Serial.println(r_pass);
  Serial.print(F("CHAN:"));
  Serial.println(e_CHAN);
  Serial.print(F("AP:"));
  Serial.println(e_AP);
  Serial.print(F("AP_ENC:"));
  Serial.println(e_ENC);
#endif

  if (eeprom_unlock)
  {
#ifdef SERV_DEBUG  
    Serial.println(F("Updating EEPROM:"));
#endif 
    strncpy(e_SSID,r_ssid,32);
    strncpy(e_PASS,r_pass,32);
    
    write_eeprom();
    defaultAP = 0;
  }

  response_send_progmem(ch_id, content_c2ee, strlenPGM(content_c2ee));
}

/*********************************************************************************************** 
*  c2ct.html 
*/
prog_char content_c2ct[] PROGMEM = "<HTML><BODY>TIME SET<hr><p><a href=index.html>Home</a></BODY></HTML>\r\n";

void page_c2ct(int ch_id, char* URL)
{
  char* Params = strstr(URL,"?")+1;

#ifdef SERV_DEBUG  
  Serial.print(F("Params:"));
  Serial.println(Params);
#endif

  char* r_hour=strstr(Params,"ch=")+3;
  char* r_min=strstr(Params,"cm=")+3;
  char* r_day=strstr(Params,"cd=")+3;
  char* r_base=strstr(Params,"cb=")+3;

  if (strstr(r_min,"&"))
    *(strstr(r_min,"&")) = 0;
  if (strstr(r_hour,"&"))
    *(strstr(r_hour,"&")) = 0;
  if (strstr(r_day,"&"))
    *(strstr(r_day,"&")) = 0;
  if (strstr(r_base,"&"))
    *(strstr(r_base,"&")) = 0;

  if (r_base) {
    sscanf(r_base, "%ld", &t_usPerSec );
    if (eeprom_unlock)
      write_eeprom();
  }
  
  sscanf(r_min, "%d", &minutes );
  sscanf(r_hour, "%d", &hours );
  sscanf(r_day, "%d", &days );

  seconds=0;
  
#ifdef SERV_DEBUG  
  Serial.print(F("New time:"));
  Serial.print(hours);
  Serial.print(F(":"));
  Serial.println(minutes);
  Serial.print(F("Day:"));
  Serial.println(days);
  Serial.print(F("Base:"));
  Serial.println(t_usPerSec);
#endif

  response_send_progmem(ch_id, content_c2ct, strlenPGM(content_c2ct));
}

/*********************************************************************************************** 
*  cdate.html 
*/
void page_cdate(int ch_id, char* URL)
{
  char buffer[80];
  
  sprintf(buffer,"<HTML><BODY>%02i:%02i:%02i on day %i</BODY></HTML>\r\n",hours,minutes,seconds,days);
    
  response_send_simple(ch_id, buffer, strlen(buffer));
}

/*********************************************************************************************** 
*  cntp.html 
*/
prog_char content_cntp[] PROGMEM = "<HTML><BODY>TIME SET<hr><p><a href=index.html>Home</a></BODY></HTML>\r\n";

void page_cntp(int ch_id, char* URL)
{
  char* Params = strstr(URL,"?")+1;

#ifdef SERV_DEBUG  
  Serial.print(F("Params:"));
  Serial.println(Params);
#endif

  char* r_ip=strstr(Params,"ip=")+3;
  char* r_tz=strstr(Params,"tz=")+3;

  if (strstr(r_ip,"&"))
    *(strstr(r_ip,"&")) = 0;
  if (strstr(r_tz,"&"))
    *(strstr(r_tz,"&")) = 0;

  if (strstr(Params,"ntp=on")) {
    e_DO_NTP=1;
  } else {
    e_DO_NTP=0;
  }
  
  sscanf(r_tz, "%d", &e_TZ );
  strncpy(e_NTP,r_ip,16);

  if (eeprom_unlock) {
    write_eeprom();
  }
  
#ifdef SERV_DEBUG  
  Serial.print(F("Set ntp:"));
  Serial.println(r_ip);
  Serial.print(F("enabled:"));
  Serial.println(e_DO_NTP);
  Serial.print(F("Timezone: UTC +"));
  Serial.println(e_TZ);
#endif

  response_send_progmem(ch_id, content_c2ct, strlenPGM(content_c2ct));
}

/*********************************************************************************************** 
*        END OF BUILT-IN Functions and html pages 
*/


