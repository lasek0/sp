#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


struct Fmt {
	char endian;
	char format;
	char print;
	uint32_t count;
	char* name;
	struct Fmt* next;
};


struct Fmt* new(struct Fmt** head) {
	if (head == NULL) return NULL;
	if (*head == NULL) {
		*head = malloc (sizeof(struct Fmt));
		(*head)->endian = '@';
		(*head)->format = 'x';
		(*head)->print = 'x';
		(*head)->count = 1;
		(*head)->name = NULL;
		(*head)->next = NULL;
		return *head;
	} else {
		for (struct Fmt* x = *head; x; x = x->next) {
			if (x->next == NULL) {
				return new (&(x->next));
			}	
		}
		//somethting went wrong
		return NULL;
	}
}

void delete(struct Fmt** head) {
	if (head == NULL) return;
	for (struct Fmt* x = *head; x;) {
		struct Fmt* y = x->next;
		memset (x, 0, sizeof(struct Fmt));
		free (x);
		x = y;
	}
	*head = NULL;
}

uint32_t size(struct Fmt* head){
	if (head == NULL) return 0;
	uint32_t cnt = 0;
	for (struct Fmt* x = head; x; x = x->next) {
		cnt++;
	}
	return cnt;
}


void endian(char e, void* d, int l) {
	uint8_t* x = (uint8_t*)d;
	if (l&1) return;
        if (x == NULL) return;

	if (e == '@' || e == '<') return; //native for x86
	if (e == '>') {
		uint8_t* z = (uint8_t*)d + l - 1;
		l >>= 1; // div 2
		for (uint8_t y; l; l--) {
			y = *z;
			*z = *x;
			*x = y;
			x++;
			z--;
		}
	}
}



int main(int argc, char* argv[]) {

	if (argc <= 1) {
		printf (
"Binary struct pack and unpack.\n"
"Based on python's struct.pack and struct.unpack\n"
"Copyright (c) 2022 by Adrian Laskowski\n"
"\n"
"Usage: %s [opt] \"fmt1[fmt2[...]]\" val1 [val2 [...]]\n"
"\n"
"  fmt:\n"
"    endian indicator (optional):\n"
"      @   native endianess (default)\n"
"      <   little endian\n"
"      >   BIG endian\n"
"    type:\n"
"      x   pad byte\n"
"      c   char\n"
"      b   int8_t\n"
"      B   uint8_t\n"
"      h   int16_t\n"
"      H   uint16_t\n"
"      i   int32_t\n"
"      I   uint32_t\n"
"      l   int32_t\n"
"      L   uint32_t\n"
"      q   int64_t\n"
"      Q   uint64_t\n"
"      f   float\n"
"      d   double\n"
"      s   c string\n"
"      p   pascal string\n"
"    array (optional):\n"
"      use \"[N]\" array notation to indicate an array of values.\n"
"\n"
"  opt:\n"
"   -r      reverse - unpack insteadof pack\n"
"   -j N    skip N first bytes. ignored for -r.\n"
"   -x XX   pad byte value. ignored for -r.\n"
"   -n STR  comma separated struct names. only with -r\n"
"           otherwise skipped, Ex: -n \"id,first name,age\"\n"
"   -p STR  print format for each fmt. only with -r\n"
"           fmt: c\n"
"             c   char\n"
"           fmt: b B h H i I l L q Q P\n"
"             i   decimal signed\n"
"             u   decimal unsigned\n"
"             x   hexadecimal (default)\n"
"             o   octal\n"
"             b   binary\n"
"           fmt: f d\n"
"             f   floating point\n"
"             d   double precision floating point\n"
"           fmt: s p\n"
"             s   string\n"
"\n"
"  val:\n"
"    Allow to pass values as numbers, string or bytes.\n"
"    Example: 123, 0b111, 0o672, 0xab1, \"def ine\"\n"
"    In case where -r is passed as option, then val1 are file name\n"
"    and val2... are skipped.\n"
"    You can pass \"-\" as file name for reading from stdin\n"
"\n"
"\n"
"Examples:\n"
"$ %s -x 0xff \"<Hx[2]>Ib[4]s\" 0x4142 0xaabbccdd 1 2 3 4 \"DEF\" | hexdump -C\n"
"00000000  42 41 ff ff aa bb cc dd  01 02 03 04 44 45 46 00  |BA..........DEF.|\n"
"00000010\n"
"\n"
"$ echo -ne \"\\x41\\x42\\x43\\x00\\xaa\\xbb\\xcc\\xdd\\x01\\x00\\x00\\x00\" |\n"
" %s -r -n \"tag,arr,val\" \"c[3]x>H[2]<I\" -\n"
"tag: ABC\n"
"arr: 0xaabb, 0xccdd\n"
"val: 0x00000001\n"
"\n"
, *argv, *argv, *argv
			);
		return -1;
	}

	struct Fmt* fmts = NULL;
	uint8_t pad_byte = 0;
	uint8_t reverse = 0;
	uint32_t skip = 0;
	char* names = NULL;
	char* print = NULL;

	// parse opt
	for (; *argv; ) {
		char* opt = *++argv;
		if (!opt) goto error2;
		if (*opt++ != '-') break;
		if (!*opt) goto error2;
		if (*argv == 0) goto error2;
		else if (*opt == 'r') reverse = 1;
		else if (*opt == 'x') pad_byte = (uint8_t)strtoul (*++argv, NULL, 0);
		else if (*opt == 'j') skip = (uint32_t)strtoul (*++argv, NULL, 0);
		else if (*opt == 'n') names = *++argv;
		else if (*opt == 'p') print = *++argv;
		else goto error2;
	}

	// parse fmt
	if (!*argv) {
		goto error1;
	}
	for (char* fmt = *argv++; fmt && *fmt; ) {
		struct Fmt* i = new(&fmts);

		// parse endian indicator
		if (strchr ("<>@", *fmt))
			i->endian = *fmt++;

		// parse format
		if (!*fmt) goto error1;
		if (strchr ("xcbBhHiIlLqQfdspP", *fmt))
			i->format = *fmt++;

		// set default print format
		switch (i->format) {
			case 'x': i->print = ' '; break;
			case 'c': i->print = 'c'; break;
			case 'b': 
			case 'B': 
			case 'h': 
			case 'H': 
			case 'i': 
			case 'I': 
			case 'l': 
			case 'L': 
			case 'q': 
			case 'Q': 
			case 'P': i->print = 'x'; break;
			case 'f': i->print = 'f'; break;
			case 'd': i->print = 'd'; break;
			case 's': 
			case 'p': i->print = 's'; break;
		}

		if (!*fmt) break;

		// parse array notaton
		if (*fmt == '['){
			fmt++;
			char *t = NULL;
			i->count = (uint32_t)strtoul (fmt, &t, 0);
			if (t && *t != ']') goto error1;
			fmt = t+1;
		}

	}



	// parse names parameter
	if (names) {
		for (struct Fmt* i = fmts; i; i = i->next) {
			if (i->format == 'x') continue;
			if (i->next != NULL) {
				char *t = strchr (names, ',');
				if (t == NULL) goto error3;
				*t = 0;
				i->name = names;
				names = t+1;
			} else {
				char *t = strchr (names, ',');
				if (t != NULL) goto error3;
				i->name = names;
				names = NULL;
			}
		}
	}

	// parse print
	if (print) {
		for (struct Fmt* i = fmts; i; i = i->next) {
			if (i->format == 'x') continue;

			if(*print == 0) goto error4;

			// validate format for fmt 
			switch (i->format) {
				case 'c':
					if(*print != 'c') goto error5; else break;
				case 'b':
				case 'h':
				case 'i':
				case 'l':
				case 'q':
					if(strchr("ixob", *print) == NULL) goto error5; else break;
				case 'B':
				case 'H':
				case 'I':
				case 'L':
				case 'Q':
				case 'P':
					if(strchr("uxob", *print) == NULL) goto error5; else break;
				case 's':
				case 'p':
					if(*print != 's') goto error5; else break;
				case 'f':
					if(*print != 'f') goto error5; else break;
				case 'd':
					if(*print != 'd') goto error5; else break;

			}

			i->print = *print;
			print++;

			if (i->next == NULL && *print != 0) goto error4;
		}
	}
	

	/*
	// XXX: DEBUG ONLY
	for (struct Fmt* i = fmts; i; i = i->next) {
		fprintf (stderr, "%c %c %c %d %s\n",
				i->endian,
				i->format,
				i->print,
				i->count,
				i->name);
	}
	*/

	// parse val
	if (reverse == 0) {
		//
		FILE* out = stdout;

		for (struct Fmt* i = fmts; i; i = i->next) {
			switch (i->format) {
				case 'x':
					for (;i->count; i->count--) {
						fwrite (&pad_byte, 1, 1, out);
					}
					break;
				case 'c':
					for (char x; i->count; i->count--) {
						x = *((*argv)++);
						fwrite (&x, 1, 1, out);
					}
					argv++;
					break;
				case 'B':
					for (uint8_t x; i->count; i->count--) {
						x = (uint8_t)strtoul (*argv++, NULL, 0);
						fwrite (&x, 1, 1, out);
					}
					break;
				case 'b':
					for (int8_t x; i->count; i->count--){
						x = (int8_t)strtol (*argv++, NULL, 0);
						fwrite (&x, 1, 1, out);
					}
					break;
				case 'H':
					for (uint16_t x; i->count; i->count--){
						x = (uint16_t)strtoul (*argv++, NULL, 0);
						endian (i->endian, &x, 2);
						fwrite (&x, 2, 1, out);
					}
					break;
				case 'h':
					for (int16_t x; i->count; i->count--){
						x = (int16_t)strtol (*argv++, NULL, 0);
						endian (i->endian, &x, 2);
						fwrite (&x, 2, 1, out);
					}
					break;
				case 'L': 
				case 'I':
					for (uint32_t x; i->count; i->count--){
						x = (uint32_t)strtoul (*argv++, NULL, 0);
						endian (i->endian, &x, 4);
						fwrite (&x, 4, 1, out);
					}
					break;
				case 'l': 
				case 'i':
					for (int32_t x;i->count; i->count--) {
						x = (int32_t)strtol (*argv++, NULL, 0);
						endian (i->endian, &x, 4);
						fwrite (&x, 4, 1, out);
					}
					break;
				case 'Q':
					for(uint64_t x;i->count; i->count--){
						x = (uint64_t)strtoull (*argv++, NULL, 0);
						endian (i->endian, &x, 8);
						fwrite (&x, 8, 1, out);
					}
					break;
				case 'q':
					for(int64_t x;i->count; i->count--){
						x = (int64_t)strtoll (*argv++, NULL, 0);
						endian (i->endian, &x, 8);
						fwrite (&x, 8, 1, out);
					}
					break;
				case 'f':
					for(float x;i->count; i->count--){
						x = (float)strtof (*argv++, NULL);
						endian (i->endian, &x, 4);
						fwrite (&x, 4, 1, out);
					}
					break;
				case 'd':
					for(double x;i->count; i->count--){
						x = (double)strtod (*argv++, NULL);
						endian (i->endian, &x, 8);
						fwrite (&x, 8, 1, out);
					}
					break;
				case 's': 
					i->count = strlen (*argv)+1;
					for(char x; i->count; i->count--){
						x = *(*argv)++;
						fwrite (&x, 1, 1, out);
					}
					argv++;
					break;
				case 'p':
					i->count = strlen (*argv);
					if (i->count > 255){
						//TODO: error
					}
					fwrite (&i->count, 1, 1, out);

					for(char x; i->count; i->count--){
						x = *(*argv)++;
						fwrite (&x, 1, 1, out);
					}
					argv++;
					break;
				default:
					  // impossible!
					break;
			}
		}
	} else {
		FILE* in = NULL;
		FILE* out = NULL;

		char* infname = *argv++;
		if (infname == NULL){
			goto error6;
		} else if (strncmp(infname, "-", 1) == 0) {
			in = stdin;
		} else {
			in = fopen(infname, "rb");
			if (in == NULL) goto error6;
		}


		char* outfname = *argv++;
		if (outfname == NULL) {
			out = stdout;
		} else {
			out = fopen(outfname, "w");
			if (in == NULL){
				fclose(in);
				goto error6;
			}
		}


		for (char dummy; skip; skip--) {
			fread (&dummy, 1, 1, in);
		}

		
		for (struct Fmt* i = fmts; i; i = i->next) {
			char print_fmt[4] = "%\0\0\0";
			print_fmt[1] = i->print;
			if (i->name) fprintf (out, "%s: ", i->name);
			if (i->print == 'd'){
				strncpy(print_fmt, "%lf", 4);
			}

			switch (i->format) {
				case 'x':
					for (char x; i->count; i->count--) {
						fread (&x, 1, 1, in);
					}
					break;
				case 'c':
					for (char x; i->count; i->count--) {
						fread (&x, 1, 1, in);
						fprintf (out, print_fmt, x);
					}
					break;
				case 'b':
					for (int8_t x; i->count; i->count--) {
						fread (&x, 1, 1, in);
						fprintf (out, print_fmt, x);
						if (i->count > 1) fprintf (out, ", ");
					}
					break;
				case 'B':
					for (uint8_t x; i->count; i->count--) {
						fread (&x, 1, 1, in);
						fprintf (out, print_fmt, x);
						if (i->count > 1) fprintf (out, ", ");
					}
					break;
				case 'h':
					for (int16_t x; i->count; i->count--) {
						fread (&x, 2, 1, in);
						endian (i->endian, &x, 2);
						fprintf (out, print_fmt, x);
						if (i->count > 1) fprintf (out, ", ");
					}
					break;
				case 'H':
					for (uint16_t x; i->count; i->count--) {
						fread (&x, 2, 1, in);
						endian (i->endian, &x, 2);
						fprintf (out, print_fmt, x);
						if (i->count > 1) fprintf (out, ", ");
					}
					break;
				case 'l':
				case 'i':
					for (int32_t x; i->count; i->count--) {
						fread (&x, 4, 1, in);
						endian (i->endian, &x, 4);
						fprintf (out, print_fmt, x);
						if (i->count > 1) fprintf (out, ", ");
					}
					break;
				case 'L':
				case 'I':
					for (uint32_t x; i->count; i->count--) {
						fread (&x, 4, 1, in);
						endian (i->endian, &x, 4);
						fprintf (out, print_fmt, x);
						if (i->count > 1) fprintf (out, ", ");
					}
					break;
				case 'q':
					for (int64_t x; i->count; i->count--) {
						fread (&x, 8, 1, in);
						endian (i->endian, &x, 8);
						fprintf (out, print_fmt, x);
						if (i->count > 1) fprintf (out, ", ");
					}
					break;
				case 'Q':
					for (uint64_t x; i->count; i->count--) {
						fread (&x, 8, 1, in);
						endian (i->endian, &x, 8);
						fprintf (out, print_fmt, x);
						if (i->count > 1) fprintf (out, ", ");
					}
					break;
				case 'f':
					for (float x; i->count; i->count--) {
						fread (&x, 4, 1, in);
						endian (i->endian, &x, 4);
						fprintf (out, print_fmt, x);
						if (i->count > 1) fprintf (out, ", ");
					}
					break;
				case 'd':
					for (double x; i->count; i->count--) {
						fread (&x, 8, 1, in);
						endian (i->endian, &x, 8);
						fprintf (out, print_fmt, x);
						if (i->count > 1) fprintf (out, ", ");
					}
					break;
			}
			if(i->format != 'x') fprintf (out, "\n");
		}

		if (strncmp(infname, "-", 1) != 0) {
			fclose(in);
		}

		if (outfname != NULL) {
			fclose(out);
		}

	}


	delete (&fmts);
	return 0;

error1:
	fprintf (stderr, "ERROR: invalid fmt format!\n");
	delete (&fmts);
	return -1;
error2:
	fprintf (stderr, "ERROR: invalid opt!\n");
	delete (&fmts);
	return -2;

error3:
	fprintf (stderr, "ERROR: invalid number of names!\n");
	delete (&fmts);
	return -3;

error4:
	fprintf (stderr, "ERROR: invalid number of print formats\n");
	delete (&fmts);
	return -4;

error5:
	fprintf (stderr, "ERROR: invalid print format\n");
	delete (&fmts);
	return -5;

error6:
	fprintf(stderr, "ERROR: could not open file.\n");
	delete (&fmts);
	return -6;
}

