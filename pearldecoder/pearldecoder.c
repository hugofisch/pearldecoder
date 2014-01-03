/*
 * pearldecoder.c:
 *	Decoder to receive mesages from a NC7367-675 temprature
 *  sensor (www.pearl.de)
 * 
 *	Copyright (c) 2014 Marc Burkhardt
 ***********************************************************************
 * This file is part of pearldecoder:
 *	http://marcraspberrypi.tumblr.com
 *
 *    pearldecoder is free software: you can redistribute it and/or 
 *    modify it under the terms of the GNU Lesser General Public License
 *    as published by the Free Software Foundation, either version 3 of 
 *    the License, or (at your option) any later version.
 *
 *    pearldecoder is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with wiringPi.
 *    If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#include <stdio.h> 
#include <stdlib.h>
#include <wiringPi.h>

#define cBITTIME 500      // Zeit in mykrosekunden für ein gültiges Signal
#define cBUFFER  2000     // Anzahl von zeichen im Meldungspuffer
#define cPIN 0            // PIN von dem der Sensor eingelesen wird

#define cMSGSIZE 36       // Anzahl von Bits in einer Meldung

#define cSTARTBIT 2       // Länge des Startzeichens
#define cSYNCGAP 19       // Länge der großen Syncpause
#define cBITONE 5         // Länge einer 1
#define cBITZERO 3        // Länge einer 0
#define cSMALLSYNCGAP 9   // Länge der kleinen Syncpause

#define cFOURBIT	4     // Vierbit Wert
#define cEIGHTBIT	8     // Achtbit Wert
#define cTWELVEBIT	12    // Zwölfbit Wert

int m_nLastRise=0;       // Zeitpunkt der letzten gestiegenen Flanke

char m_strMsg[cBUFFER+1]; // Meldungspuffer
int m_nMsgIndex=0;       // Schreibindex fgür den Puffer


int m_MsgFlag_S=0;       // Flag, wenn der Start erkannt wurde
int m_MsgFlag_g=0;       // Flag, wenn eine kleine Synclücke erkannt wurde       
int m_MsgFlag_G=0;       // Flag, wenn ein große Synclücke erkannt wurde
int m_nMsgBitCounter=0;  // Zähler der erkannten meldungsbits

// Decodiert einen Binären String in eine Zahl. Länge
// der Binärzahl kann angegeben werden, so wie die Startposition im String
unsigned int DecodeSingleValue(int nStart, int nBase)
{
	unsigned int unReturn=0;
	int i=0;
	unsigned int unValue=0;
	
	// Sicherheitsabfrage
	if ((nStart <0)|| ((nStart+nBase-1)>cBUFFER)) return 0;
	if (nBase> 32) return 0;
	
	
	// Bits umrechnen in Werte
	for (i=0;i<nBase;i++)
	{
		// nur die gesetzten Bits interessieren
		if (m_strMsg[nStart+i]=='1')
		{
			// eine 1 um Anzahl schiften
			unValue=1<<((nBase-1)-i);
			// und dann mit dem Rückgabewert verodern
			unReturn=unReturn | unValue;
		}
	}		
	
	return unReturn;
	
}

// Dumpmethode für den Binärstring
void Dump()
{
	// wir derzeit nicht genutzt (für spätere Debugzwecke
	// Einfach den Inhalt als 4er Blocks ausgeben
	int i=0;	
	for (i=0;i<9;i++)
	{
	  printf("%c%c%c%c\n",m_strMsg[i*4],m_strMsg[i*4+1],m_strMsg[i*4+2],m_strMsg[i*4+3]);
	}
	printf("\n");	
}


// Ausgabe der decodierten Meldung
void Decode()
{
	unsigned int unSenderkennung=0;
	unsigned int unKanal=0;
	unsigned int unTemperatur=0;
	unsigned int unRest=0;

	// Senderkennung
	unSenderkennung=DecodeSingleValue(0,cEIGHTBIT);
	// Kanal
	unKanal=DecodeSingleValue(8,cFOURBIT);
	// Temperatur
	unTemperatur=DecodeSingleValue(12,cTWELVEBIT);
	// Rest
	unRest=DecodeSingleValue(24,cTWELVEBIT);

    printf("Decoder %0X - ",unSenderkennung);
    printf("Channel %0X - ",unKanal);
    printf("Temp %0.1f - ",unTemperatur/10.0);
	printf("Rest %0X\n",unRest);
}

// Ein Zeichen in den Meldungspuffer schreiben
void AddCharToMsg(char cValue)
{
	// Zeichen speichern
	m_strMsg[m_nMsgIndex]=cValue;
	// hinter dem Zeichen Null setzen (besser bei der Ausgabe
	m_strMsg[m_nMsgIndex+1]=0;
	// Zeiger inkrementiern
	m_nMsgIndex++;
	// Puffergrenze prüfen und wieder von vorn anfangen. Sollte nicht passieren
	// aber besser die Grenzen abfangen
	if (m_nMsgIndex==cBUFFER) m_nMsgIndex=0;
}

// Meldungspuffer und alle Flags etc. zurück setzen
void ResetMsg()
{
	// Alles zurücksetzen
	m_nMsgIndex=0;
	m_MsgFlag_S=0;
	m_MsgFlag_g=0;
	m_MsgFlag_G=0;
	m_nMsgBitCounter=0;	
	m_strMsg[m_nMsgIndex]=0;
}


// Interruptroutine bei Erkennung der steigenden Flanke
void SensorInterrupt (void) 
{
  // schnell den Zeitpunkt merken
  int nRise=micros();
  // Nun die Lücke ausrechnen. Einfach die Zeit vom letzten Mal
  // bis jetzt durch die Länge eines Bits (0,5ms) teilen und runden.
  int nGap=(int)((float)(nRise-m_nLastRise)/cBITTIME +0.5);
  m_nLastRise=nRise; // Zeitpunkt der Flanke merken

  switch (nGap)
  {
	  case cSTARTBIT: // Startlücke erkannt
	    // Daten gehen neu los. Also Reset.
		ResetMsg();
	    m_MsgFlag_S=1;
	    break;
	  case cBITZERO: // Nulllücke erkannt
	    // gilt aber nur, wenn grosse Synclücke vorher erkannt war
	    if (m_MsgFlag_G==1)
	    {
			AddCharToMsg('0');
			m_nMsgBitCounter++;
		}
	    break;
	  case cBITONE: // Einslücke erkannt
	    // gilt aber nur, wenn grosse Synclücke vorher erkannt war
	    if (m_MsgFlag_G==1)
	    {
			AddCharToMsg('1');
			m_nMsgBitCounter++;
		}
	    break;	  
	  case cSMALLSYNCGAP: // kleine Synclücke erkannt
	    // gilt aber nur, wenn grosse Synclücke vorher erkannt war
	    if (m_MsgFlag_G==1)
	    {
			m_MsgFlag_g++;
		}
	    break;	  
	  case cSYNCGAP: // grosse Synclücke erkannt
	    // gilt aber nur wenn Start erkannt war
	    if (m_MsgFlag_S==1)
	    {
			m_MsgFlag_G=1;
		}
        break;
	  default: // keine Ahnung. Müll wird ignoriert
	    break;
  }


  // Wann haben wir einen volständigen Datensatz empfangen ?
  // Wenn wir einmal S haben, einmal G, 36 Bits und ein g.
  // Besser ist es wenn wir zwei mal 36 Bits und zwei mal g hätten.
  // So könnten wir Störungen ausfiltern. Wenn beide Blöcke identisch sind
  // dann sollten die Daten mit genügend großer Wahrscheinlichkeit OK sein
  // Wenn man es ganz genau haben will, dann kann man auch alle 12 Blöcke prüfen
  
  if ((m_MsgFlag_S==1)&&(m_MsgFlag_G==1)&&(m_MsgFlag_g==2)&&(m_nMsgBitCounter==(2*cMSGSIZE)))
  {
	  // Meldung erkannt 
	  int nIndex=0;
	  // Konsitenz check
	  while(nIndex < cMSGSIZE)
	  {
		  // Sind die Inhalte in beiden Abschnitten gleich ?
		  if (m_strMsg[nIndex]!=m_strMsg[nIndex+cMSGSIZE]) nIndex=cBUFFER;
		  nIndex++;
	  }
	  
      if (nIndex==cMSGSIZE)
      {
		  // Beide Datenstreams waren indentisch
		  // dekodiren
      	  Decode();
	  }
      else
      {
		  // Fehler in den Daten
		  // Reset
		  ResetMsg();
	  }
  }

}


/*
 *********************************************************************************
 * main
 *********************************************************************************
 */

int main (void)
{
	printf("Decoder V0.5 for NC-7367-675 Pearl Tempreture Sensor 433MHz\n");
	printf("by Marc Burkhardt ");
	printf("powered by wirinPi ");
	printf("GNU Licence\n\n");
	wiringPiSetup () ;
	wiringPiISR (cPIN, INT_EDGE_RISING, &SensorInterrupt) ;
	while (1==1){delay(1000);};
	return 0;
}




