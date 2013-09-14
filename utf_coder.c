/*
Matrikelnummer: 25289
Name: Christian Schade
Fuer: Programmieren 1 - Angewandte Informatik - WS 08/09
Version: 23.1.09
*/

/*
__________TODO_____________
- Fehlerueberpruefung fuer argv Parameter
- BOM ueberpruefen

	Kodierung 	Bytefolge
	UTF-8 	EF BB BF
	UTF-16 Big Endian 	FE FF
	UTF-16 Little Endian 	FF FE

- unnoetige Masken entfernen

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Masken
  const unsigned char
  b00000000=0x00, b10000000=0x80, b11000000=0xC0, b11100000=0xE0, b11110000=0xF0,
  b11111000=0xF8, b11111100=0xFC, b11111110=0xFE, b00000001=0x01, b00000011=0x03,
  b00000111=0x07, b00001111=0x0F, b00011111=0x1F, b00111111=0x3F, b01111111=0x7F,
  b00011100=0x1C, b11011000=0xD8, b11111111=0xFF;
  long int utfcode_lang;


void fail(){
  fprintf(stderr,"-> Fehlerhafte UTF-Zeichenkette. Beende das Programm.\n");
  abort();
}

int checkBOM(FILE *quelle){ //  1 UTF BOM (nicht notwendig), 2 Little Endian, 3 Big Endian, 4 Kein BOM gefunden
  int z = 0;
  int bom[3];
  for (z ; z < 3; z++){
    bom[z] = fgetc(quelle);
  }

  if (bom[0] == 0xFE && bom[1] == 0xFF){
    return 2; // LE
  }

    else if (bom[0] == 0xFF && bom[1] == 0xFE){
      return 3; // BE
    }

	  else if (bom[0] == 0xEF && bom[1] == 0xBB && bom[2] == 0xBF ){
        return 3; // UTF 8 mit oder ohne BOM
      }

	    else {
	      return 4; // kein BOM
	    }

}


int checkARGV(FILE *quelle, char *zielformat){// Ueberprueft die uebergebenen ARGV Parameter
                                              // 0 ok 1 Quelle fehlt 2 ungueltiges Zielformat

  if (quelle==NULL){
    fprintf(stderr,"Quelldatei existiert nicht.");
    return 1;
  }

  else if ( (strcmp(zielformat, "utf8") != 0) && (strcmp(zielformat, "utf16") != 0) ) {
    fprintf(stderr,"Kein gueltiges Zielformat - waehlen Sie entweder utf8 oder utf16.");
    return 2;
  }

  else{
    return 0;
  }

}



void utf8to16(FILE *quelle, FILE *ziel ){
  /*
  // UTF 8 -> UTF 16 (LE)
  // 0xxxxxxx                             ->    xxxxxxxx xxxxxxxx
  // 110xxxxx                             ->    xxxxxxxx xxxxxxxx
  // 110xxxxx 10xxxxxx                    ->    xxxxxxxx xxxxxxxx
  // 1110xxxx 10xxxxxx 10xxxxxx           ->    xxxxxxxx xxxxxxxx
  // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx  ->    110110xx xxxxxxxx 110111xx xxxxxxxx
  */

  /*
  // Kodierung 	Bytefolge
  // UTF-8 	EF BB BF
  // UTF-16 Big Endian 	FE FF
  // UTF-16 Little Endian 	FF FE
  */

  int src[4];
  int target[4];
  int z = 0; // nur Testzwecke

  while (1){ // Laeuft bis Zeichenkodierung zu Ende oder Abbruch durch Fehler fail()

    src[0] = fgetc(quelle); // Erstes Zeichen lesen und nachfolgend schauen,
                            // mit wie vielen Bytes das Zeichen kodiert ist.
    z = z+1;

    if (src[0]==EOF){
      break;
    }

    // 0xxxxxxx    ->    xxxxxxxx  xxxxxxxx

    if ((src[0]&b10000000)==0x00) {
    //    if ((((char)src[0])&b10000000)==0) {

      if (src[0]==0) { // Abfangen vom NULL-Byte (notwendig durch fgetc)
        fprintf(stderr,"NULL-Byte -> Abbruch\n");
        abort();
      }

      fputc(src[0], ziel);
      fputc(0x00, ziel); // WARUM ???


      continue;
    }



    // 110xxxxx 10xxxxxx    ->    xxxxxxxx xxxxxxxx
    else if ((src[0]&b11100000)==b11000000) {
      src[1] = fgetc(quelle);

      if ((src[1]&b11000000)==b10000000) {
        // 0x07 = b00000111
        target[0] = ((src[0] >> 2) & 0x07 ); //+ (src[1] & 0x07);
        printf("\nZeichen 1: %x", target[0]);

        // 0x3F = b00111111
        target[1] = (src[1] & 0x3F) + (src[0] << 6);
        printf("\nZeichen 2: %x", src[1]);

	// Schreiben nach Little Endian
        fputc(target[1], ziel);
        fputc(target[0], ziel);
        }

      else {
       fail();
      }
    }


    // 1110xxxx 10xxxxxx 10xxxxxx  ->    xxxxxxxx xxxxxxxx
    else if ((src[0]&b11110000)==b11100000) {
      src[1] = fgetc(quelle);

      if ((src[1]&b11000000)==b10000000) {
        src[2] = fgetc(quelle);

        if ((src[2]&b11000000)==b10000000) {

	  //
          target[0] = (((src[1] & b00111111) >> 2) + ((src[0] & b00001111) << 4));
          printf("\nZeichen 1: %x", target[0]);

          // 0x3F = b00111111
          target[1] = ((src[2] & b00111111) + (src[1] << 6));
          printf("\nZeichen 2: %x", src[1]);

          fputc(target[1], ziel);
          fputc(target[0], ziel);

	}
      }

      else {
        fail();
      }
    }
    // 21 aenderbare Bits fuer Zeichen -> 20
    // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx ->    110110xx xxxxxxxx  110111xx xxxxxxxx      // Low- / High Surrogate  (=> Little Endian)
    else if ((src[0]&b11111000)==b11110000) {
      src[1] = fgetc(quelle);

      if ((src[1]&b11000000)==b10000000) {
        src[2] = fgetc(quelle);

        if ((src[2]&b11000000)==b10000000) {
          src[3] = fgetc(quelle);

          if ((src[3]&b11000000)==b10000000) {

            // 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx -> ++++++++ +++xxx\xx xxxxxxxx xxxxxxxx
 	    // utfcode_lang = ((src[0] & 0x07) << 18) + ((src[0] & 0x3F) << 12) +((src[1] & 0x3F) << 6) + (src[2] & 0x3F);
	    // der unicode wert funktioniert soweit :D
	    utfcode_lang = ((src[0] & 0x07) << 18) +
		           ((src[1] & 0x3F) << 12) +
			   ((src[2] & 0x3F) << 6) +
			   (src[3] & 0x3F);

	    utfcode_lang = utfcode_lang - 0x10000; // eliminiert das 21. Bit ;)
	    //printf("\nUTFcode:   %x\n", utfcode_lang);

            target[0] = ((utfcode_lang & 0x000C0000) >> 18)+0xD8;
            target[1] = (utfcode_lang & 0x0003FC00)>> 10;
            //  target[2] = ((utfcode_lang & 0x00000300) >> 18)+0xDC;
            target[2] = ((utfcode_lang & 0x00000300) >> 8)+0xDC;
            target[3] = utfcode_lang & 0x000000FF;


	    //
            //         target[0] = (((src[0] >>2) >> 2) + ((src[0] & b00001111) << 4));
            printf("\nZeichen 1: %x", target[0]);

            // 0x3F = b00111111
            // target[1] = ((src[2] & b00111111) + (src[1] << 6));
            printf("\nZeichen 2: %x", src[1]);

            fputc(target[1], ziel);
            fputc(target[0], ziel);
	    fputc(target[3], ziel);
            fputc(target[2], ziel);

          }
        }
      }

      else {
        fail();
      }

    }

  }
}



void utf16to8(FILE *quelle, FILE *ziel){
 /*
  // UTF 16 -> UTF 8 (LE)
  // xxxxxxxx xxxxxxxx                    ->     0xxxxxxx (0000 0000 - 0000 007F)
  // xxxxxxxx xxxxxxxx                    ->     110xxxxx 10xxxxxx (0000 0080 - 0000 07FF)
  // xxxxxxxx xxxxxxxx                    ->     1110xxxx 10xxxxxx 10xxxxxx (0000 0800 - 0000 FFFF)
  // xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx  ->     11110xxx 10xxxxxx 10xxxxxx 10xxxxxx (0001 0000 - 0010 FFFF)

  */

  /*
  // Kodierung 	Bytefolge
  // UTF-8 	EF BB BF
  // UTF-16 Big Endian 	FE FF
  // UTF-16 Little Endian 	FF FE
  */

  int src[4];
  int target[4];
  int z = 0; // nur Testzwecke
  unsigned int unicode = 0; // 4 Bytes lang
  unsigned int unicode2 = 0; // 4 Bytes lang

  while (1){ // Laeuft bis Zeichenkodierung zu Ende oder Abbruch durch Fehler fail()

    src[0] = fgetc(quelle);  // Erstes Zeichen lesen und nachfolgend schauen,
                             // mit wie vielen Bytes das Zeichen kodiert ist.
    src[1] = fgetc(quelle);  // besser mit ftell & fseek ?
    src[2] = fgetc(quelle);
    src[3] = fgetc(quelle);

    z = z+1;

    if (src[0]==EOF || src[1]==EOF || src[2]==EOF || src[3]==EOF){ // war 0
      break;printf("EOF");
    }

    // 0xxxxxxx    ->    xxxxxxxx  xxxxxxxx

// hiermit funktionierts , aber nur ein Zeichen
/*
    if ((src[1]&b11111111)==0x00) {


      fputc(src[1], ziel);
      fputc(0x00, ziel); // WARUM ???
      printf("00 einfuegen");


      continue;
    }
*/

      // if ((src[1] >= 0xDC && src[1] < 0xDF) && (src[3] >= 0xD8 && src[3] <= 0xDB)) { // 4 Bytes durch Low Surrogate ? (LE)

      //if (src[1] >= 0xD8 && src[1] < 0xE0){
      if (src[1] >= 0xD8 && src[1] < 0xDB){ // 4 Bytes

      unicode = (src[1] & 0x03); unicode = unicode << 18;
      unicode = (src[0] & 0xFF); unicode = unicode << 10;
      unicode = (src[3] & 0x03); unicode = unicode <<  8;
      unicode = (src[2] & 0xFF);
      unicode = unicode + 0x10000;

      printf("\n\n  >>> Unicode:::: < %i >", unicode);

      //LE
      /*
      fputc(target[1], ziel);
      fputc(target[0], ziel);
	  fputc(target[3], ziel);
      fputc(target[2], ziel);
      */
      }

      else {// 2 Unicodes weil bereits 4 Bytes ausgelesen und kein Surrogate
        unicode = src[1];  unicode = unicode << 8;
        unicode = src[0];

        unicode2 = src[3]; unicode2 = unicode2 << 8;
        unicode2 = src[2];

        printf("\n\n  >>> !Unicode:: < %i >", unicode);
        printf("\n\n  >>> !Unicode:: < %i >", unicode2);


          if (unicode <= 0x7F){
            target[0]= unicode &b01111111;
            fputc(target[0], ziel);
          }

          else if (unicode >= 0x80 && unicode <= 0x07FF){

            target[0]= unicode&0x0000003F;
            target[1]= ((unicode&0x000006C0)<<2)+0x0000C080;
            fputc(target[0], ziel);
            fputc(target[1], ziel);

            //fputc((unicode &0x0000003F)+((unicode&0x000006C0)<<2) + 0x0000C080, ziel);
          }

          else if (unicode >= 0x0800 && unicode <= 0xFFFF){
            target[1]= (unicode>>12) & 0x0000000F;
            target[2]= (unicode>>6)  & 0x0000003F;
            target[3]= (unicode & 0x0000003F) + b10000000;

            fputc(target[1], ziel);
            fputc(target[2], ziel);
            fputc(target[3], ziel);
            printf("hieeeeeeeeeeeeeeeeeeeeer richtig");
          }



          if (unicode2 <= 0x7F){
            target[3]= unicode2 & b01111111;
            fputc(target[3], ziel);
          }

          else if (unicode2 >= 0x80 && unicode2 <= 0x07FF){

            target[2]= unicode2&0x0000003F;
            target[3]= ((unicode2&0x000006C0)<<2)+0x0000C080;
            fputc(target[2], ziel);
            fputc(target[3], ziel);

            //fputc((unicode &0x0000003F)+((unicode&0x000006C0)<<2) + 0x0000C080, ziel);
          }

          else if (unicode2 >= 0x0800 && unicode2 <= 0xFFFF){
            target[1]= (unicode2>>12) & 0x0000000F;
            target[2]= (unicode2>>6)  & 0x0000003F;
            target[3]= (unicode2 & 0x0000003F) + b10000000;

            fputc(target[1], ziel);
            fputc(target[2], ziel);
            fputc(target[3], ziel);
          }
      }

  }

}



int main (int argc, char* argv[]){
  FILE *quelle;
  FILE *ziel;

  char* zielformat = argv[1];
  char* quellort = argv[2];
  char* zielort = argv[3];

  quelle = fopen(quellort,"r");
  ziel = fopen(zielort,"wb");

  if(checkARGV(quelle, zielformat) == 0)
  {
  printf("bindrin");


// printf("%i",checkBOM(quelle));



  //if (checkBOM(quelle) == 4 || checkBOM(quelle) == 1){// UTF 8 -> UTF 16
    //freopen(quellort,"r",quelle);
    if(strcmp(zielformat, "utf8")== 0){
      utf16to8(quelle,ziel);
    }

    else if (strcmp(zielformat, "utf16")== 0){
      utf8to16(quelle,ziel);
    }

    else {
      fprintf(stderr,"Kein gueltiges Zielformat - waehlen Sie entweder utf8 oder utf16.");
    }
//  }

  }
  fclose(quelle);
  fclose(ziel);


return 0;
}