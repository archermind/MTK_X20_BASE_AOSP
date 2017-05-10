#pragma once
#include "functions/scatter/IScatterFileReader.h"

class yaml_reader : public IScatterFileReader
{
public:
	BOOL get_scatter_file(string scatter_file_name, /*IN OUT*/scatter_file_struct* scatter_file);
   static BOOL verify(string scatter_file_name);
};
