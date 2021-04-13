 /*
 Copyright (c) 2016, akm
 All rights reserved.
 This content is under the MIT License.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dosbox.h"
#include "control.h"
#include "support.h"
#include "jega.h"//for AX
using namespace std;

#define SBCS 0
#define DBCS 1
#define ID_LEN 6
#define NAME_LEN 8
#define SBCS19_LEN 256 * 19
#define DBCS16_LEN 65536 * 32

Bit8u jfont_sbcs_19[SBCS19_LEN];//256 * 19( * 8)
Bit8u jfont_dbcs_16[DBCS16_LEN];//65536 * 16 * 2 (* 8)

typedef struct {
    char id[ID_LEN];
    char name[NAME_LEN];
    unsigned char width;
    unsigned char height;
    unsigned char type;
} fontx_h;

typedef struct {
    Bit16u start;
	Bit16u end;
} fontxTbl;

Bit16u chrtosht(FILE *fp)
{
	Bit16u i, j;
	i = (Bit8u)getc(fp);
	j = (Bit8u)getc(fp) << 8;
	return(i | j);
}

Bitu getfontx2header(FILE *fp, fontx_h *header)
{
    fread(header->id, ID_LEN, 1, fp);
    if (strncmp(header->id, "FONTX2", ID_LEN) != 0) {
	return 1;
    }
    fread(header->name, NAME_LEN, 1, fp);
    header->width = (Bit8u)getc(fp);
    header->height = (Bit8u)getc(fp);
    header->type = (Bit8u)getc(fp);
    return 0;
}

void readfontxtbl(fontxTbl *table, Bitu size, FILE *fp)
{
    while (size > 0) {
	table->start = chrtosht(fp);
	table->end = chrtosht(fp);
	++table;
	--size;
    }
}

static void LoadFontxFile(const char * fname) {
    fontx_h head;
    fontxTbl *table;
    Bitu code;
    Bit8u size;
	if (!fname) return;
	if(*fname=='\0') return;
	FILE * mfile=fopen(fname,"rb");
	if (!mfile) {
		LOG_MSG("MSG: Can't open FONTX2 file: %s",fname);
		return;
	}
	if (getfontx2header(mfile, &head) != 0) {
		fclose(mfile);
		LOG_MSG("MSG: FONTX2 header is incorrect\n");
		return;
    }
	// switch whether the font is DBCS or not
	if (head.type == DBCS) {
		if (head.width == 16 && head.height == 16) {
			size = getc(mfile);
			table = (fontxTbl *)calloc(size, sizeof(fontxTbl));
			readfontxtbl(table, size, mfile);
			for (Bitu i = 0; i < size; i++)
				for (code = table[i].start; code <= table[i].end; code++)
					fread(&jfont_dbcs_16[(code * 32)], sizeof(Bit8u), 32, mfile);
		}
		else {
			fclose(mfile);
			LOG_MSG("MSG: FONTX2 DBCS font size is not correct\n");
			return;
		}
	}
    else {
		if (head.width == 8 && head.height == 19) {
			fread(jfont_sbcs_19, sizeof(Bit8u), SBCS19_LEN, mfile);
		}
		else {
			fclose(mfile);
			LOG_MSG("MSG: FONTX2 SBCS font size is not correct\n");
			return;
		}
    }
	fclose(mfile);
}

void JFONT_Init(Section_prop * section) {
	std::string file_name;
	Prop_path* pathprop = section->Get_path("jfontsbcs");
	if (pathprop) LoadFontxFile(pathprop->realpath.c_str());
	else LOG_MSG("MSG: SBCS font file path is not specified.\n");
	pathprop = section->Get_path("jfontdbcs");
	if(pathprop) LoadFontxFile(pathprop->realpath.c_str());
	else LOG_MSG("MSG: DBCS font file path is not specified.\n");
}
