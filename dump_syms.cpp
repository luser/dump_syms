#include <stdio.h>

#include "PDBParser.h"

int main(int argc, char** argv)
{
	if (argc != 2) {
		fprintf(stderr, "Usage: dump_syms <pdb file>\n");
		return 1;
	}
	google_breakpad::PDBParser parser;
	parser.load(argv[1]);
	parser.printBreakpadSymbols(stdout);
	return 0;
}