/*
 * pearlquick.c:
 *	Quick test prgamm to receive mesages from a NC7367-675 temprature
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
#include <wiringPi.h>

// Konstanten
#define BITTIME 500
#define PIN 0

// globale Varialen
int m_nBitCounter=0;  // Zähler für den 4er Block
int m_nLastRise=0;    // Zeitpunkt des letzten IRQs
// Interruptmethode
void SensorInterrupt (void) 
{
  // schnell den Zeitpunkt merken
  int nRise=micros();
  // Nun die Lücke ausrechnen. Einfach die Zeit vom letzten Mal
  // bis jetzt durch die Länge eines Bits (0,5ms) teilen und runden.
  int nGap=(int)((float)(nRise-m_nLastRise)/BITTIME +0.5);
  m_nLastRise=nRise; // Zeitpunkt der Flanke merken

  switch (nGap)
  {
	  case 2: // Startlücke
	    printf("\nS\n");
	    fflush(stdout);	    
	    m_nBitCounter=0; // Neuer 4er Block
	    break;
	  case 3: // Nulllücke
	    printf("0 "); 
	    m_nBitCounter++; // Stelle erhöhen
	    break;
	  case 5: // Einslücke
	    printf("1 "); 
	    m_nBitCounter++; // Stelle erhöhen
	    break;	  
	  case 9: // kleine Synclücke 
	    printf("\ng\n");
	    fflush(stdout);
	    m_nBitCounter=0; // Neuer 4er Block 
	    break;	  
	  case 19: // grosse Synclücke
	    printf("\nG\n");
	    fflush(stdout);	    
	    m_nBitCounter=0; // Neuer 4er Block  
	    break;
	  default: // keine Ahnung. Müll
	    //printf("\n?\n");
	    //m_nBitCounter=0; // Neuer 4er Block 
	    break;
  }
  // Wenn 4er Block komplett, dann neue Zeile
  if (m_nBitCounter==4){
	   m_nBitCounter=0; // Neuer 4er Block  
	   printf("\n");
   }
}

// MAIN Routine
int main (void)
{
  // WiringPi initialisieren
  wiringPiSetup () ;
  // Interrupt auslösen, wenn an PIN (hier Pin 0), die Signalflanke steigt.
  wiringPiISR (PIN, INT_EDGE_RISING, &SensorInterrupt) ;

  // endlos nix tun
  while (1==1){delay(1000);};
  return 0;
}




