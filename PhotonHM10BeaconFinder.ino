/**************************************************************************/
/*!
    @file     PhotonHM10BeaconFinder.ino
    @author   scout01

    @section LICENSE

    Software License Agreement (BSD License)

    Copyright (C) 2016 scout01 (Daniel Pohl)
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:
    1. Redistributions of source code must retain the above copyright
       notice, this list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright
       notice, this list of conditions and the following disclaimer in the
       documentation  and/or other materials provided with the distribution.
    3. Neither the names of the copyright holders nor the names of any
       contributors may be used to endorse or promote products derived from this
       software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
    AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
    ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
    LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
    CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
    SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
    INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
    CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
    ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
    POSSIBILITY OF SUCH DAMAGE.

*/
/**************************************************************************/

#define serialSpeed 9600
#define debugMode false
#define readTimeout 5000

// Buffer which contents all data which comes in during a wait for response
String msgBuff = "";

void setup() {
  // The HM10 needs to be set into the correct working mode for beacon scanning.
  log("Begin BTLE", false);
  Particle.process();

  Serial1.begin(9600);

  Serial1.print("AT+RENEW");
  waitForSerialEvt("OK+RENEW");
  log("at+renew: " + msgBuff, true);

  Serial1.print("AT+RESET");
  waitForSerialEvt("OK+RESET");
  log("at+reset: " + msgBuff, true);

  Serial1.print("AT+IMME1");
  waitForSerialEvt("OK+SET:1");
  log("at+imme1: " + msgBuff, true);

  Serial1.print("AT+ROLE1");
  waitForSerialEvt("OK+SET:1");
  log("at+role1: " + msgBuff, true);
  log("BTLE setup done - begin scanning", false);
}


boolean toggle = false;
void loop() {
  Serial1.print("AT+DISI?");
  if (waitForSerialEvt("OK+DISCE")) {
    Particle.process();
    processResponseAndPublish(msgBuff);
  } else {
    // We do not try to parse the buffer if the termination command could not be found
    log("Module does not respond like expected - Expected: OK+DISCE - Buffer content is logged next...", false);
    log(msgBuff, false);
  }
  Particle.process();
  delay(2000);
}

void processResponseAndPublish(String content) {
  /* The format is (Without line breaks):
  OK+DISISOK
  +DISC:00000000:00000000000000000000000000000000:0000000000:FFFFFFFFFFFF:-085OK
  +DISC:00000000:00000000000000000000000000000000:0000000000:FFFFFFFFFFFF:-087OK
  +DISC:00000000:00000000000000000000000000000000:0000000000:FFFFFFFFFFFF:-038OK
  +DISCE
  */
  int index = 0;
  int loopies = 0;
  String beacons = "[";
  while(index > -1) {
    loopies++;
    index = content.indexOf("+DISC:", index);
    if (index < 0) {
      break;
    }
    String signalStrength = content.substring(index + 72, index + 76);
    String mac = content.substring(index + 59, index + 71);
    if (beacons.length() > 1) {
      beacons.concat(", ");
    }
    beacons.concat("{\"mac\":\"" + mac + "\", \"signal\":\"" + signalStrength + "\"}");
    Particle.process();

    if (loopies > 20) {
      index = -1;
      break;
    }
    index = index + 4;
  }
  if (beacons.length() > 1) {
    beacons.concat("]");
      Particle.publish("BEACONS-FOUND", beacons, 60, PRIVATE);
  }
}

/*
  Log to the cloud for debugging and information puroposes.
  message: A string which containts the text to be logged.
  debug:   If true, the content is only logged when the defined degug flag is also true
*/
void log(String message, boolean debug) {
  if (!debug || debugMode) {
    Particle.publish("BLE-Log", message, 60, PRIVATE);
  }
}

/*
  The name is a bit confusing. "waitFor" seems to be a reseverd method name.
  waitForText: The termination command on which we should wait to respond.

  There is also a defined timeout to prevent a device blocking
*/
bool waitForSerialEvt(String waitForText) {
  uint16_t timeout = readTimeout;
  msgBuff = "";
  bool result = false;
  while (timeout--) {
    int readChar = -1;
    while ((readChar = Serial1.read()) > -1) {
      msgBuff.concat(String(char(readChar)));
      if (msgBuff.endsWith(waitForText)) {
        timeout = 0;
        result = true;
        // This was a try to remove the termination command.
        // Except the termination we have much more overhead into the response.
        // So this cut of will only cost time we won't waste.
        //msgBuff.remove(msgBuff.length() - 1 - waitForText.length());
        break;
      }
    }
    if (timeout == 0) {
      break;
    }
    Particle.process();
    delay(1);
  }
  return result;
}
