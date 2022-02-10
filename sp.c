#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


struct Fmt {
	char endian;
	char format;
	char* print;
	uint32_t count;
	void* data;
	uint32_t size;
	char* name;
	struct Fmt* next;
};


struct Fmt* new(struct Fmt** head) {
	if (head == NULL) return NULL;
	if (*head == NULL) {
		*head = malloc (sizeof(struct Fmt));
		if (*head == NULL) return NULL;
		(*head)->endian = '@';
		(*head)->format = 0;
		(*head)->print = "";
		(*head)->count = 1;
		(*head)->data = NULL;
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
		if (x->data)
			free(x->data);
		memset (x, 0, sizeof(struct Fmt));
		free (x);
		x = y;
	}
	*head = NULL;
}

/*
 * curently unused
uint32_t size(struct Fmt* head){
	if (head == NULL) return 0;
	uint32_t cnt = 0;
	for (struct Fmt* x = head; x; x = x->next) {
		cnt++;
	}
	return cnt;
}
*/

void endian(char e, void* d, int l) {
	uint8_t* x = (uint8_t*)d;
	if (l&1) return;
        if (x == NULL) return;

	// only for LE X86
	if (e == '@' || e == '<') return;
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

	//TODO: for BE arch
}


const char* help;

int main(int argc, char* argv[]) {

	if (argc <= 1) {
		printf (help, *argv, *argv, *argv);
		return -1;
	}

	struct Fmt* fmts = NULL;
	uint8_t pad_byte = 0;
	uint8_t reverse = 0;
	char* names = NULL;
	uint32_t max_name_size = 0;
	char* print = NULL;
        char* infn = NULL;
        char* outfn = NULL;
	FILE* in = stdin;
	FILE* out = stdout;
	enum { ERR_OPT_LIST=1, ERR_UNK_OPT, ERR_MISS_FMT, ERR_MISS_FMT_CHR,
		ERR_ARR_FMT, ERR_NAME_OPT, ERR_NAME_TOO_FEW, ERR_NAME_TOO_MUCH,
		ERR_PRINT_OPT, ERR_PRINT_TOO_FEW, ERR_PRINT_INV_FMT, ERR_PRINT_TOO_MUCH,
		ERR_OPEN_IN_FILE, ERR_OPEN_OUT_FILE, ERR_PASCAL_STR_LEN,
		ERR_IN_NAME_ALLOW, ERR_VALS_COUNT, ERR_ALLOC, ERR_READ_IN, 
	};

	// parse opt
	for (; *argv; ) {
		char* opt = *++argv;
		if (!opt){
			fprintf (stderr, "ERROR: unexpected end of opt list!\n");
			delete (&fmts);
			return ERR_OPT_LIST;
		}
		if (*opt++ != '-') break;
		else if (*opt == 'r') reverse = 1;
		else if (*opt == 'x') pad_byte = (uint8_t)strtoul (*++argv, NULL, 0);
		else if (*opt == 'n') names = *++argv;
		else if (*opt == 'p') print = *++argv;
		else if (*opt == 'i') infn = *++argv;
		else if (*opt == 'o') outfn = *++argv;
		else {
			fprintf (stderr, "ERROR: unknown parameter '%c'\n", *opt);
			delete (&fmts);
			return ERR_UNK_OPT;
		}
	}

	// parse fmt
	if (!*argv) {
		fprintf (stderr, "ERROR: missing fmt!\n");
		delete (&fmts);
		return ERR_MISS_FMT;
	}
	for (char* fmt = *argv++; fmt && *fmt; ) {
		struct Fmt* i = new(&fmts);
		if (i == NULL) {
			fprintf (stderr, "ERROR: Could not allocate memory!\n");
			delete (&fmts);
			return ERR_ALLOC;
		}

		// parse endian indicator
		if (strchr ("<>@", *fmt))
			i->endian = *fmt++;

		// parse format
		if (!*fmt) {
			fprintf (stderr, "ERROR: missing fmt char!\n");
			delete (&fmts);
			return ERR_MISS_FMT_CHR;
		}
		if (strchr ("xcbBhHiIqQfdsp", *fmt))
			i->format = *fmt++;

		// set default print format
		switch (i->format) {
			case 'x': i->print = ""; break;
			case 'c': i->print = "%c"; break;
			case 'b': 
			case 'B': 
			case 'h': 
			case 'H': 
			case 'i': 
			case 'I': 
			case 'q': 
			case 'Q': 
			case 'P': i->print = "%x"; break;
			case 'f': i->print = "%f"; break;
			case 'd': i->print = "%lf"; break;
			case 's': 
			case 'p': i->print = "%s"; break;
		}

		// parse array notaton
		if (*fmt == '['){
			fmt++;
			char *t = NULL;
			i->count = strtoul (fmt, &t, 0);
			if (t && *t != ']') {
				fprintf (stderr, "ERROR: invalid array notation format!\n");
				delete (&fmts);
				return ERR_ARR_FMT;
			}
			if (i->count > 65535 || i->count < 1) {
				fprintf (stderr, "ERROR: array size '%u' invalid\n", i->count);
				delete (&fmts);
				return ERR_ARR_FMT;
			}
			fmt = t+1;
		}

	}



	// parse names parameter
	if (names && reverse == 0) {
		fprintf (stderr, "ERROR: -n allow only with -r");
		delete (&fmts);
		return ERR_NAME_OPT;
	}
	if (names && reverse == 1) {
		for (struct Fmt* i = fmts; i; i = i->next) {
			if (i->format == 'x') continue;
			if (i->next != NULL) {
				char *t = strchr (names, ',');
				if (t == NULL) {
					fprintf (stderr, "ERROR: too few names\n");
					delete (&fmts);
					return ERR_NAME_TOO_FEW;
				}
				*t = 0;
				i->name = names;
				size_t l = strlen (i->name);
				if (l > max_name_size) max_name_size = l;
				names = t+1;
			} else {
				char *t = strchr (names, ',');
				if (t != NULL) {
					fprintf (stderr, "ERROR: too much names\n");
					delete (&fmts);
					return ERR_NAME_TOO_MUCH;
				}
				i->name = names;
				size_t l = strlen (i->name);
				if (l > max_name_size) max_name_size = l;
				names = NULL;
			}
		}
	}

	// parse print
	if (print && reverse == 0) {
		fprintf (stderr, "ERROR: -p allowed only with -r\n");
		delete (&fmts);
		return ERR_PRINT_OPT;
	}
	if (print && reverse == 1) {
		for (struct Fmt* i = fmts; i; i = i->next) {
			if (i->format == 'x') continue;

			if(*print == 0) {
				fprintf (stderr, "ERROR: too few print formats\n");
				delete (&fmts);
				return ERR_PRINT_TOO_FEW;
			}

			// validate format for fmt 
			switch (i->format) {
				case 'c':
					if(*print != 'c') {
						fprintf (stderr, "ERROR: invalid print format '%c' for '%c' fmt\n", i->format, *print);
						delete (&fmts);
						return ERR_PRINT_INV_FMT;
					}
					i->print = "%c";
					break;
				case 'b':
				case 'h':
				case 'i':
				case 'l':
				case 'q':
					switch (*print) {
						case 'i': i->print = "%i"; break;
						case 'x': i->print = "%x"; break;
						case 'o': i->print = "%o"; break;
						case 'b': i->print = "%b"; break;
						default: {
							fprintf (stderr, "ERROR: invalid print format '%c' for '%c' fmt\n", i->format, *print);
							delete (&fmts);
							return ERR_PRINT_INV_FMT;
						}

					}
					break;
				case 'B':
				case 'H':
				case 'I':
				case 'L':
				case 'Q':
					switch (*print) {
						case 'u': i->print = "%u"; break;
						case 'x': i->print = "%x"; break;
						case 'o': i->print = "%o"; break;
						case 'b': i->print = "%b"; break;
						default: {
							fprintf (stderr, "ERROR: invalid print format '%c' for '%c' fmt\n", i->format, *print);
							delete (&fmts);
							return ERR_PRINT_INV_FMT;
						}
					}
					break;
				case 'f':
					if(*print != 'f') {
						fprintf (stderr, "ERROR: invalid print format '%c' for '%c' fmt\n", i->format, *print);
						delete (&fmts);
						return ERR_PRINT_INV_FMT;
					}
					i->print = "%f";
					break;
				case 'd':
					if(*print != 'd') {
						fprintf (stderr, "ERROR: invalid print format '%c' for '%c' fmt\n", i->format, *print);
						delete (&fmts);
						return ERR_PRINT_INV_FMT;
					}
					i->print = "%lf";
					break;
				case 's':
				case 'p':
					if(*print != 's') {
						fprintf (stderr, "ERROR: invalid print format '%c' for '%c' fmt\n", i->format, *print);
						delete (&fmts);
						return ERR_PRINT_INV_FMT;
					}
					i->print = "%c";
					break;

			}

			print++;

			if (i->next == NULL && *print != 0) {
				fprintf (stderr, "ERROR: too much print formats char.");
				delete (&fmts);
				return ERR_PRINT_TOO_MUCH;
			}
		}
	}
	

	// parse in file name
	if (infn && reverse == 0) {
		fprintf (stderr, "ERROR: -i allowed only with -r\n");
		delete (&fmts);
		return ERR_IN_NAME_ALLOW;
	}
	if (infn && reverse == 1) {
		in = fopen (infn, "rb");
		if (!in) {
			fprintf (stderr, "ERROR: could not open file '%s'\n", infn);
			delete (&fmts);
			return ERR_OPEN_IN_FILE;
		}
	}

	// parse out file name
	if (outfn) {
		out = fopen (outfn, "w");
		if (!out) {
			fprintf (stderr, "ERROR: could not open file '%s'\n", outfn);
			delete (&fmts);
			return ERR_OPEN_OUT_FILE;
		}
	}

	// parse val
	if (reverse == 0) {
		for (struct Fmt* i = fmts; i; i = i->next) {
			switch (i->format) {
				case 'x':
					i->size = sizeof(uint8_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset(i->data, pad_byte, i->size);
					break;
				case 'c':
					if (*argv == NULL) {
						fprintf (stderr, "ERROR: not enough val params\n");
						delete (&fmts);
						return ERR_VALS_COUNT;
					}
					i->size = sizeof(char) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						char* t = i->data;
						t[k] = (*argv)[k];
						//TODO: think about validating chars
					}
					argv++;
					break;
				case 'b':
					i->size = sizeof(int8_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						if (*argv == NULL) {
							fprintf (stderr, "ERROR: not enough val params\n");
							delete (&fmts);
							return ERR_VALS_COUNT;
						}
						int8_t* t = i->data;
						t[k] = strtol (*argv++, NULL, 0);
					}
					break;
				case 'B':
					i->size = sizeof(uint8_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						if (*argv == NULL) {
							fprintf (stderr, "ERROR: not enough val params\n");
							delete (&fmts);
							return ERR_VALS_COUNT;
						}
						uint8_t* t = i->data;
						t[k] = strtoul (*argv++, NULL, 0);
					}
					break;
				case 'h':
					i->size = sizeof(int16_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						if (*argv == NULL) {
							fprintf (stderr, "ERROR: not enough val params\n");
							delete (&fmts);
							return ERR_VALS_COUNT;
						}
						int16_t* t = i->data;
						t[k] = strtol (*argv++, NULL, 0);
						endian (i->endian, &t[k], 2);
					}
					break;
				case 'H':
					i->size = sizeof(uint16_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						if (*argv == NULL) {
							fprintf (stderr, "ERROR: not enough val params\n");
							delete (&fmts);
							return ERR_VALS_COUNT;
						}
						uint16_t* t = i->data;
						t[k] = strtoul (*argv++, NULL, 0);
						endian (i->endian, &t[k], 2);
					}
					break;
				case 'i':
					i->size = sizeof(int32_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						if (*argv == NULL) {
							fprintf (stderr, "ERROR: not enough val params\n");
							delete (&fmts);
							return ERR_VALS_COUNT;
						}
						int32_t* t = i->data;
						t[k] = strtol (*argv++, NULL, 0);
						endian (i->endian, &t[k], 4);
					}
					break;
				case 'I':
					i->size = sizeof(uint32_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						if (*argv == NULL) {
							fprintf (stderr, "ERROR: not enough val params\n");
							delete (&fmts);
							return ERR_VALS_COUNT;
						}
						uint32_t* t = i->data;
						t[k] = strtoul (*argv++, NULL, 0);
						endian (i->endian, &t[k], 4);
					}
					break;
				case 'q':
					i->size = sizeof(int64_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						if (*argv == NULL) {
							fprintf (stderr, "ERROR: not enough val params\n");
							delete (&fmts);
							return ERR_VALS_COUNT;
						}
						int64_t* t = i->data;
						t[k] = strtoll (*argv++, NULL, 0);
						endian (i->endian, &t[k], 8);
					}
					break;
				case 'Q':
					i->size = sizeof(uint64_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						if (*argv == NULL) {
							fprintf (stderr, "ERROR: not enough val params\n");
							delete (&fmts);
							return ERR_VALS_COUNT;
						}
						uint64_t* t = i->data;
						t[k] = strtoull (*argv++, NULL, 0);
						endian (i->endian, &t[k], 8);
					}
					break;
				case 'f':
					i->size = sizeof(float) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						if (*argv == NULL) {
							fprintf (stderr, "ERROR: not enough val params\n");
							delete (&fmts);
							return ERR_VALS_COUNT;
						}
						float* t = i->data;
						t[k] = strtof (*argv++, NULL);
						endian (i->endian, &t[k], 4);
					}
					break;
				case 'd':
					i->size = sizeof(double) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						if (*argv == NULL) {
							fprintf (stderr, "ERROR: not enough val params\n");
							delete (&fmts);
							return ERR_VALS_COUNT;
						}
						double* t = i->data;
						t[k] = strtod (*argv++, NULL);
						endian (i->endian, &t[k], 8);
					}
					break;
				case 's': 
					for (uint32_t k = 0; k < i->count; k++) {
						if (*argv == NULL) {
							fprintf (stderr, "ERROR: not enough val params\n");
							delete (&fmts);
							return ERR_VALS_COUNT;
						}
						int len = strlen (*argv) + 1;
						i->size += len;
						i->data = realloc (i->data, i->size);
						if (i->data == NULL) {
							fprintf (stderr, "ERROR: could not allocate memory\n");
							delete (&fmts);
							return ERR_ALLOC;
						}
						memcpy ((char*)i->data + i->size - len, *argv, len);
						argv++;
					}
					break;
				case 'p': 
					for (uint32_t k = 0; k < i->count; k++) {
						if (*argv == NULL) {
							fprintf (stderr, "ERROR: not enough val params\n");
							delete (&fmts);
							return ERR_VALS_COUNT;
						}
						int len = strlen (*argv);
						if (len > 255) {
							fprintf (stderr, "ERROR: pascal string length '%d' too large\n", len);
							delete (&fmts);
							return ERR_ALLOC;
						}
						i->size += len + 1;
						i->data = realloc (i->data, i->size);
						if (i->data == NULL) {
							fprintf (stderr, "ERROR: could not allocate memory\n");
							delete (&fmts);
							return ERR_ALLOC;
						}
						*((uint8_t*)i->data + i->size - len - 1) = len;
						memcpy ((char*)i->data + i->size - len, *argv, len);
						argv++;
					}
					break;
			}
		}
	} else {
		for (struct Fmt* i = fmts; i; i = i->next) {
			switch (i->format) {
				case 'x':
					for (uint32_t k = 0; k < i->count; k++) {
						char c;
						fread (&c, 1, 1, in);
					}
					break;
				case 'c':
					i->size = sizeof(char) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: Could not allocate memory!\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						char* t = i->data;
						int r = fread (&t[k], sizeof(char), 1, in);
						if (r != 1) {
							fprintf (stderr, "ERROR: could not read data from input file\n");
							delete (&fmts);
							return ERR_READ_IN;
						}
						//TODO: think about validating chars
					}
					break;
				case 'b':
					i->size = sizeof(int8_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						int8_t* t = i->data;
						int r = fread (&t[k], sizeof(int8_t), 1, in);
						if (r != 1) {
							fprintf (stderr, "ERROR: could not read data from input file\n");
							delete (&fmts);
							return ERR_READ_IN;
						}
					}
					break;
				case 'B':
					i->size = sizeof(uint8_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						uint8_t* t = i->data;
						int r = fread (&t[k], sizeof(uint8_t), 1, in);
						if (r != 1) {
							fprintf (stderr, "ERROR: could not read data from input file\n");
							delete (&fmts);
							return ERR_READ_IN;
						}
					}
					break;
				case 'h':
					i->size = sizeof(int16_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						int16_t* t = i->data;
						int r = fread (&t[k], sizeof(int16_t), 1, in);
						if (r != 1) {
							fprintf (stderr, "ERROR: could not read data from input file\n");
							delete (&fmts);
							return ERR_READ_IN;
						}
						endian(i->endian, &t[k], 2);
					}
					break;
				case 'H':
					i->size = sizeof(uint16_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						uint16_t* t = i->data;
						int r = fread (&t[k], sizeof(uint16_t), 1, in);
						if (r != 1) {
							fprintf (stderr, "ERROR: could not read data from input file\n");
							delete (&fmts);
							return ERR_READ_IN;
						}
						endian(i->endian, &t[k], 2);
					}
					break;
				case 'i':
					i->size = sizeof(int32_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						int32_t* t = i->data;
						int r = fread (&t[k], sizeof(int32_t), 1, in);
						if (r != 1) {
							fprintf (stderr, "ERROR: could not read data from input file. '%d'\n", r);
							delete (&fmts);
							return ERR_READ_IN;
						}
						endian(i->endian, &t[k], 4);
					}
					break;
				case 'I':
					i->size = sizeof(uint32_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						uint32_t* t = i->data;
						int r = fread (&t[k], sizeof(uint32_t), 1, in);
						if (r != 1) {
							fprintf (stderr, "ERROR: could not read data from input file\n");
							delete (&fmts);
							return ERR_READ_IN;
						}
						endian(i->endian, &t[k], 4);
					}
					break;
				case 'q':
					i->size = sizeof(int64_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						int64_t* t = i->data;
						int r = fread (&t[k], sizeof(int64_t), 1, in);
						if (r != 1) {
							fprintf (stderr, "ERROR: could not read data from input file\n");
							delete (&fmts);
							return ERR_READ_IN;
						}
						endian(i->endian, &t[k], 8);
					}
					break;
				case 'Q':
					i->size = sizeof(uint64_t) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						uint64_t* t = i->data;
						int r = fread (&t[k], sizeof(uint64_t), 1, in);
						if (r != 1) {
							fprintf (stderr, "ERROR: could not read data from input file\n");
							delete (&fmts);
							return ERR_READ_IN;
						}
						endian(i->endian, &t[k], 8);
					}
					break;
				case 'f':
					i->size = sizeof(float) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						float* t = i->data;
						int r = fread (&t[k], sizeof(float), 1, in);
						if (r != 1) {
							fprintf (stderr, "ERROR: could not read data from input file\n");
							delete (&fmts);
							return ERR_READ_IN;
						}
						endian(i->endian, &t[k], 4);
					}
					break;
				case 'd':
					i->size = sizeof(double) * i->count;
					i->data = malloc (i->size);
					if (i->data == NULL) {
						fprintf (stderr, "ERROR: could not allocate memory\n");
						delete (&fmts);
						return ERR_ALLOC;
					}
					memset (i->data, 0, i->size);
					for (uint32_t k = 0; k < i->count; k++) {
						double* t = i->data;
						int r = fread (&t[k], sizeof(double), 1, in);
						if (r != 1) {
							fprintf (stderr, "ERROR: could not read data from input file\n");
							delete (&fmts);
							return ERR_READ_IN;
						}
						endian(i->endian, &t[k], 8);
					}
					break;
				case 's': {
					uint32_t off = 0;
					for (uint32_t k = 0; k < i->count; k++) {
						uint32_t len = 0;
						for (;;) {
							if (i->size <= len) {
								i->size += 0x10;
								i->data = realloc (i->data, i->size);
								if (i->data == NULL) {
									fprintf (stderr, "ERROR: could not allocate memory\n");
									delete (&fmts);
									return ERR_ALLOC;
								}
							}
							char x = 0;
							int r = fread (&x, sizeof(char), 1, in);
							if (r != 1) {
								fprintf (stderr, "ERROR: could not read data from input file\n");
								delete (&fmts);
								return ERR_READ_IN;
							}
							char* t = i->data;
							t[off + len] = x;
							len += 1;
							if (x == 0) break;
						}
						off += len;
					}
					
					break; }
				case 'p': {
					for (uint32_t k = 0; k < i->count; k++) {
						uint16_t l = 0;
						int r = fread (&l, sizeof(uint8_t), 1, in);
						if (r != 1) {
							fprintf (stderr, "ERROR: could not read data from input file\n");
							delete (&fmts);
							return ERR_READ_IN;
						}

						i->size += l + 1; // incl nul byte 
						i->data = realloc (i->data, i->size);
						if (i->data == NULL) {
							fprintf (stderr, "ERROR: could not allocate memory\n");
							delete (&fmts);
							return ERR_ALLOC;
						}

						char* t = i->data;
						r = fread (t + i->size - l - 1, sizeof(char), l, in);
						if (r != l) {
							fprintf (stderr, "ERROR: could not allocate memory\n");
							delete (&fmts);
							return ERR_ALLOC;
						}
						*(t + i->size - 1) = 0; // put null byte
					}
					break;}
			}
		}
	}

	/*
	// XXX: DEBUG ONLY
	for (struct Fmt* i = fmts; i; i = i->next) {
		fprintf (stderr, "%c %c %3s %d %s %d %p\n",
				i->endian,
				i->format,
				i->print,
				i->count,
				i->name,
				i->size,
				i->data);
	}
	*/


	// print results
	if (reverse == 0) {
		for (struct Fmt* i = fmts; i; i = i->next) {
			fwrite (i->data, i->size, 1, out);
		}
	} else {
		for (struct Fmt* i = fmts; i; i = i->next) {
			uint8_t* d = i->data;
			int r;
			if (i->name && strlen(i->name) && i->format != 'x'){
				r = fprintf(out, "%s", i->name);
				for (uint32_t x = max_name_size - r; x; x--) fprintf(out, " ");
				fprintf (out, ": ");
			}

			for (uint32_t k = 0; k < i->count; k++) {
				switch (i->format){
					case 'x':
						break;
					case 'c':
						r = fprintf(out, i->print, *(char*)d);
						d += sizeof(char);
						break;
					case 'b':
						r = fprintf(out, i->print, *(int8_t*)d);
						d += sizeof(int8_t);
						break;
					case 'B':
						r = fprintf(out, i->print, *(uint8_t*)d);
						d += sizeof(uint8_t);
						break;
					case 'h':
						r = fprintf(out, i->print, *(int16_t*)d);
						d += sizeof(int16_t);
						break;
					case 'H':
						r = fprintf(out, i->print, *(uint16_t*)d);
						d += sizeof(uint16_t);
						break;
					case 'i':
						r = fprintf(out, i->print, *(int32_t*)d);
						d += sizeof(int32_t);
						break;
					case 'I':
						r = fprintf(out, i->print, *(uint32_t*)d);
						d += sizeof(uint32_t);
						break;
					case 'q':
						r = fprintf(out, i->print, *(int64_t*)d);
						d += sizeof(int64_t);
						break;
					case 'Q':
						r = fprintf(out, i->print, *(uint64_t*)d);
						d += sizeof(uint64_t);
						break;
					case 'f':
						r = fprintf(out, i->print, *(float*)d);
						d += sizeof(float);
						break;
					case 'd':
						r = fprintf(out, i->print, *(double*)d);
						d += sizeof(double);
						break;
					case 's':
					case 'p':
						r = fprintf(out, i->print, (char*)d);
						d += r + 1;
						break;

				}
				if (i->count > 1 && k < i->count-1 && i->format != 'x' && i->format != 'c')
					fprintf (out, ", ");
			}
			if (i->format != 'x') fprintf (out, "\n");
			
		}
	}

	fclose(in);
	fclose(out);
	delete (&fmts);
	return 0;

}


const char* help = 
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
"      q   int64_t\n"
"      Q   uint64_t\n"
"      f   float\n"
"      d   double\n"
"      s   c string\n"
"      p   pascal string\n"
"    array (optional):\n"
"      use \"[N]\" array notation to indicate an array of values.\n"
"      N is limited up to 65535\n"
"\n"
"  opt:\n"
"   -r      reverse - unpack insteadof pack\n"
"   -i STR  input stream file (stdin by default). only with -r\n"
"   -o STR  output stream file (stdout by default)\n"
"   -x XX   pad byte value. ignored for -r.\n"
"   -n STR  comma separated struct names for each fmt (exclude x). only with -r\n"
"           otherwise skipped, Ex: -n \"id,first name,age\"\n"
"   -p STR  print format for each fmt. only with -r\n"
"           fmt: c\n"
"             c   char\n"
"           fmt: b h i q\n"
"             i   decimal signed\n"
"             x   hexadecimal (default)\n"
"             o   octal\n"
"             b   binary\n"
"           fmt: B H I Q\n"
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
"    For -r option, all val's are ignored.\n"
"\n"
"\n"
"Examples:\n"
"$ %s -x 0xff \"<Hx[2]>Ib[4]s\" 0x4142 0xaabbccdd 1 2 3 4 \"DEF\" | hexdump -C\n"
"00000000  42 41 ff ff aa bb cc dd  01 02 03 04 44 45 46 00  |BA..........DEF.|\n"
"00000010\n"
"\n"
"$ echo -ne \"\\x41\\x42\\x43\\x00\\xaa\\xbb\\xcc\\xdd\\x01\\x00\\x00\\x00\" |\n"
" %s -r -n \"tag,arr,val\" \"c[3]x>H[2]<I\"\n"
"tag: ABC\n"
"arr: aabb, ccdd\n"
"val: 1\n"
"\n"
;
