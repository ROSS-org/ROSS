/*=========================================================================

Program:   Visualization Toolkit
Module:    $RCSfile: vtkPhastaReader.cxx,v $

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPhastaReader.h"

#include "vtkByteSwap.h"
#include "vtkCellType.h"   //added for constants such as VTK_TETRA etc...
#include "vtkDataArray.h"
#include "vtkIntArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPointSet.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"
//change
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkPVXMLElement.h"
#include "vtkPVXMLParser.h"

#include "vtkCellData.h"
#include "vtkFieldData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"

//change end
vtkCxxRevisionMacro(vtkPhastaReader, "$Revision: 1.11 $");
vtkStandardNewMacro(vtkPhastaReader);

#include <vtkstd/map>
#include <vtkstd/vector>
#include <vtkstd/string>
#include <vtksys/ios/sstream>

//CHANGE////////////////////////////////////////////////////////////////
#include "rdtsc.h"
#define clockRate 2670000000.0
unsigned long long start, end;
//double opentime_total = 0.0;
int LAST_FILE_ID;

void startTimer(unsigned long long* start) {
	*start =  rdtsc();
}

void endTimer(unsigned long long* end) {
	*end = rdtsc();
}

void computeTime(unsigned long long* start, unsigned long long* end) {
	double time = (double)((*end-*start)/clockRate);
	opentime_total += time;
}


#define VERSION_INFO_HEADER_SIZE 8192
#define DB_HEADER_SIZE 1024
#define TWO_MEGABYTE 2097152
#define ENDIAN_TEST_NUMBER 12180 // Troy's Zip Code!!
#define MAX_PHASTA_FILES 64
#define MAX_PHASTA_FILE_NAME_LENGTH 1024
#define MAX_FIELDS_NUMBER 48
#define MAX_FIELDS_NAME_LENGTH 128
#define DefaultMHSize (4*1024*1024)
int MasterHeaderSize = DefaultMHSize;
int diff_endian = 0;
long long counter = 0;

enum PhastaIO_Errors
{
	MAX_PHASTA_FILES_EXCEEDED = -1,
	UNABLE_TO_OPEN_FILE = -2,
	NOT_A_MPI_FILE = -3,
	GPID_EXCEEDED = -4,
	DATA_TYPE_ILLEGAL = -5,
};

//CHANGE END////////////////////////////////////////////////////////////
struct vtkPhastaReaderInternal
{
	struct FieldInfo
	{
		int StartIndexInPhastaArray;
		int NumberOfComponents;
		int DataDependency; // 0-nodal, 1-elemental
		vtkstd::string DataType; // "int" or "double"
		vtkstd::string PhastaFieldTag;

		FieldInfo() : StartIndexInPhastaArray(-1), NumberOfComponents(-1), DataDependency(-1), DataType(""), PhastaFieldTag("")
		{
		}
	};

	typedef vtkstd::map<vtkstd::string, FieldInfo> FieldInfoMapType;
	FieldInfoMapType FieldInfoMap;
};


// Begin of copy from phastaIO

//CHANGE////////////////////////////////////////////////////////////////

/***********************************************************************/
/***************** NEW PHASTA IO CODE STARTS HERE **********************/
/***********************************************************************/

int partID_counter;

typedef struct
{
	bool Wrong_Endian;                            /* default to false */

	char filename[MAX_PHASTA_FILE_NAME_LENGTH];   /* defafults to 1024 */
	int nppp;
	int nPPF;
	int nFiles;
	int nFields;
	unsigned long long my_offset;

	char * master_header;
	double * double_chunk;
	int * int_chunk;
	double * read_double_chunk;
	int * read_int_chunk;
	unsigned long long **my_offset_table;
	unsigned long long **my_read_table;
	int field_count;
	int part_count;
	int read_field_count;
	int read_part_count;
	int GPid;
	int start_id;
	unsigned long long next_start_address;

	int myrank;
	int numprocs;
	int local_myrank;
	int local_numprocs;
} phastaio_file_t;

//default: Paraview disabled

typedef struct
{
	int fileID;
	int nppf, nfields;
	int GPid;
	int read_field_count;
	char * masterHeader;
	unsigned long long **offset_table;
	unsigned long long my_offset;

}serial_file;

serial_file *SerialFile;
phastaio_file_t *PhastaIOActiveFiles[MAX_PHASTA_FILES];
int PhastaIONextActiveIndex = 0; /* indicates next index to allocate */

//CHANGE END////////////////////////////////////////////////////////////

#define swap_char(A,B) { ucTmp = A; A = B ; B = ucTmp; }

vtkstd::map< int , char* > LastHeaderKey;
vtkstd::vector< FILE* > fileArray;
vtkstd::vector< int > byte_order;
vtkstd::vector< int > header_type;
int DataSize=0;
int LastHeaderNotFound = 0;
int Wrong_Endian = 0 ;
int Strict_Error = 0 ;
int binary_format = 0;

// the caller has the responsibility to delete the returned string
char* vtkPhastaReader::StringStripper( const char  istring[] )
{
	int length = strlen( istring );
	char* dest = new char [ length + 1 ];
	strcpy( dest, istring );
	dest[ length ] = '\0';

	if ( char* p = strpbrk( dest, " ") )
	{
		*p = '\0';
	}

	return dest;
}

int vtkPhastaReader::cscompare( const char teststring[],
		const char targetstring[] )
{

	char* s1 = const_cast<char*>(teststring);
	char* s2 = const_cast<char*>(targetstring);

	while( *s1 == ' ') { s1++; }
	while( *s2 == ' ') { s2++; }
	while( ( *s1 )
			&& ( *s2 )
			&& ( *s2 != '?')
			&& ( tolower( *s1 )==tolower( *s2 ) ) )
	{
		s1++;
		s2++;
		while( *s1 == ' ') { s1++; }
		while( *s2 == ' ') { s2++; }
	}
	if ( !( *s1 ) || ( *s1 == '?') )
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

void vtkPhastaReader::isBinary( const char iotype[] )
{
	char* fname = StringStripper( iotype );
	if ( cscompare( fname, "binary" ) )
	{
		binary_format = 1;
	}
	else
	{
		binary_format = 0;
	}
	delete [] fname;

}

size_t vtkPhastaReader::typeSize( const char typestring[] )
{
	char* ts1 = StringStripper( typestring );

	if ( cscompare( "integer", ts1 ) )
	{
		delete [] ts1;
		return sizeof(int);
	}
	else if ( cscompare( "double", ts1 ) )
	{
		delete [] ts1;
		return sizeof( double );
	}
	else if ( cscompare( "float", ts1 ) )
	{
		delete [] ts1;
		return sizeof( float );
	}
	else
	{
		delete [] ts1;
		fprintf(stderr,"unknown type : %s\n",ts1);
		return 0;
	}
}

int vtkPhastaReader::readHeader( FILE*       fileObject,
		const char  phrase[],
		int*        params,
		int         expect )
{
	char* text_header;
	char* token;
	char Line[1024];
	char junk;
	int FOUND = 0 ;
	int real_length;
	int skip_size, integer_value;
	int rewind_count=0;

	if( !fgets( Line, 1024, fileObject ) && feof( fileObject ) )
	{
		rewind( fileObject );
		clearerr( fileObject );
		rewind_count++;
		fgets( Line, 1024, fileObject );
	}

	while( !FOUND  && ( rewind_count < 2 ) )
	{
		if ( ( Line[0] != '\n' ) && ( real_length = strcspn( Line, "#" )) )
		{
			text_header = new char [ real_length + 1 ];
			strncpy( text_header, Line, real_length );
			text_header[ real_length ] =static_cast<char>(NULL);
			token = strtok ( text_header, ":" );
			if( cscompare( phrase , token ) )
			{
				FOUND = 1 ;
				token = strtok( NULL, " ,;<>" );
				skip_size = atoi( token );
				int i;
				for( i=0; i < expect && ( token = strtok( NULL," ,;<>") ); i++)
				{
					params[i] = atoi( token );
				}
				if ( i < expect )
				{
					fprintf(stderr,"Expected # of ints not found for: %s\n",phrase );
				}
			}
			else if ( cscompare(token,"byteorder magic number") )
			{
				if ( binary_format )
				{
					fread((void*)&integer_value,sizeof(int),1,fileObject);
					fread( &junk, sizeof(char), 1 , fileObject );
					if ( 362436 != integer_value )
					{
						Wrong_Endian = 1;
					}
				}
				else
				{
					fscanf(fileObject, "%d\n", &integer_value );
				}
			}
			else
			{
				/* some other header, so just skip over */
				token = strtok( NULL, " ,;<>" );
				skip_size = atoi( token );
				if ( binary_format)
				{
					fseek( fileObject, skip_size, SEEK_CUR );
				}
				else
				{
					for( int gama=0; gama < skip_size; gama++ )
					{
						fgets( Line, 1024, fileObject );
					}
				}
			}
			delete [] text_header;
		}

		if ( !FOUND )
		{
			if( !fgets( Line, 1024, fileObject ) && feof( fileObject ) )
			{
				rewind( fileObject );
				clearerr( fileObject );
				rewind_count++;
				fgets( Line, 1024, fileObject );
			}
		}
	}

	if ( !FOUND )
	{
		fprintf(stderr, "Error: Cound not find: %s\n", phrase);
		return 1;
	}
	return 0;
}

void vtkPhastaReader::SwapArrayByteOrder_( void* array,
		int   nbytes,
		int   nItems )
{
	/* This swaps the byte order for the array of nItems each
		 of size nbytes , This will be called only locally  */
	int i,j;
	unsigned char ucTmp;
	unsigned char* ucDst = (unsigned char*)array;

	for(i=0; i < nItems; i++)
	{
		for(j=0; j < (nbytes/2); j++)
		{
			swap_char( ucDst[j] , ucDst[(nbytes - 1) - j] );
		}
		ucDst += nbytes;
	}
}

//CHANGE///////////////////////////////////////////////////////

void vtkPhastaReader::queryphmpiio_(const char filename[],int *nfields, int *nppf)
{

	FILE * fileHandle;
	char* fname = StringStripper( filename );

	fileHandle = fopen (fname,"rb");
	if (fileHandle == NULL ) {
		printf("\n File %s doesn't exist! Please check!\n",fname);
    exit(1);
  }
	else
	{
		SerialFile =(serial_file *)calloc( 1,  sizeof( serial_file) );

		SerialFile->masterHeader = (char *)malloc(MasterHeaderSize);
		fread(SerialFile->masterHeader,1,MasterHeaderSize,fileHandle);

		char read_out_tag[MAX_FIELDS_NAME_LENGTH];
		char * token;
		int magic_number;
		memcpy( read_out_tag,
				SerialFile->masterHeader,
				MAX_FIELDS_NAME_LENGTH-1 );

		if ( cscompare ("MPI_IO_Tag",read_out_tag) )
		{
			// Test endianess ...
			memcpy ( &magic_number,
					SerialFile->masterHeader+sizeof("MPI_IO_Tag :"),
					sizeof(int) );

			if ( magic_number != ENDIAN_TEST_NUMBER )
			{
				diff_endian = 1;
			}

			char version[MAX_FIELDS_NAME_LENGTH/4];
			int mhsize;

			memcpy(version,
					SerialFile->masterHeader + MAX_FIELDS_NAME_LENGTH/2,
					MAX_FIELDS_NAME_LENGTH/4 - 1); //TODO: why -1?

			if( cscompare ("version",version) )
			{
				// if there is "version" tag in the file, then it is newer format
				// read master header size from here, otherwise use default
				// TODO: if version is "1", we know mhsize is at 3/4 place...

				token = strtok(version, ":");
				token = strtok(NULL, " ,;<>" );
				int iversion = atoi(token);

				if( iversion == 1) {
					memcpy( &mhsize,
							SerialFile->masterHeader + MAX_FIELDS_NAME_LENGTH/4*3 + sizeof("mhsize : ")-1,
							sizeof(int));
					if ( diff_endian)
						SwapArrayByteOrder_(&mhsize, sizeof(int), 1);
					free(SerialFile->masterHeader);
					SerialFile->masterHeader = (char *)malloc(mhsize);
					fseek(fileHandle, 0, SEEK_SET);
					fread(SerialFile->masterHeader,1,mhsize,fileHandle);
				}
				//TODO: check if this is a valid int??
				MasterHeaderSize = mhsize;
			}
			else { // else it's version 0's format w/o version tag, implicating MHSize=4M
				MasterHeaderSize = DefaultMHSize;
				//printf("-----> version = 0; mhsize = %d\n", MasterHeaderSize);
			}

			// END OF CHANGE FOR VERSION
			//
			memcpy( read_out_tag,
					SerialFile->masterHeader+MAX_FIELDS_NAME_LENGTH+1,
					MAX_FIELDS_NAME_LENGTH );

			// Read in # fields ...
			token = strtok ( read_out_tag, ":" );
			token = strtok( NULL," ,;<>" );
			*nfields = atoi( token );
			SerialFile->nfields=*nfields;

			memcpy( read_out_tag,
					SerialFile->masterHeader+
					*nfields * MAX_FIELDS_NAME_LENGTH +
					MAX_FIELDS_NAME_LENGTH * 2,
					MAX_FIELDS_NAME_LENGTH);

			token = strtok ( read_out_tag, ":" );
			token = strtok( NULL," ,;<>" );
			*nppf = atoi( token );
			SerialFile->nppf=*nppf;
		}
		else
		{
			printf("The file you opened is not new format, please check!\n");
		}

		fclose(fileHandle);
	}
	delete [] fname;

}

void vtkPhastaReader::finalizephmpiio_( int *fileDescriptor )
{
  //printf("total open time is %lf\n", opentime_total);
	// free master header, offset table [][], and serial file struc
	free( SerialFile->masterHeader);
	int j;
	for ( j = 0; j < SerialFile->nfields; j++ )
	{
		free( SerialFile->offset_table[j] );
	}
	free( SerialFile->offset_table);
	free( SerialFile );
}

char* StrReverse(char* str)
{
	char *temp, *ptr;
	int len, i;

	temp=str;
	for(len=0; *temp !='\0';temp++, len++);

	ptr=(char*)malloc(sizeof(char)*(len+1));

	for(i=len-1; i>=0; i--)
		ptr[len-i-1]=str[i];

	ptr[len]='\0';
	return ptr;
}


//CHANGE END//////////////////////////////////////////////////

void vtkPhastaReader::openfile( const char filename[],
		const char mode[],
		int*  fileDescriptor )
//CHANGE////////////////////////////////////////////////////
{
	//printf("in open(): counter = %ld\n", counter++);

	FILE* file=NULL ;
	*fileDescriptor = 0;
	char* fname = StringStripper( filename );
	char* imode = StringStripper( mode );

	int string_length = strlen( fname );
	char* buffer = (char*) malloc ( string_length+1 );
	strcpy ( buffer, fname );
	buffer[ string_length ] = '\0';

	char* tempbuf = StrReverse(buffer);
	free(buffer);
	buffer = tempbuf;

	//printf("buffer is %s\n",buffer);

	char* st2 = strtok ( buffer, "." );
	//st2 = strtok (NULL, ".");

	//printf("st2 is %s\n",st2);

	string_length = strlen(st2);
	char* buffer2 = (char*)malloc(string_length+1);
	strcpy(buffer2,st2);
	buffer2[string_length]='\0';

	char* tempbuf2 = StrReverse(buffer2);
	free(buffer2);
	buffer2 = tempbuf2;
	//printf("buffer2 is %s\n",buffer2);

	SerialFile->fileID = atoi(buffer2);
	if ( char* p = strpbrk(buffer, "@") )
		*p = '\0';

  startTimer(&start);
	if ( cscompare( "read", imode ) ) file = fopen(fname, "rb" );
	else if( cscompare( "write", imode ) ) file = fopen(fname, "wb" );
	else if( cscompare( "append", imode ) ) file = fopen(fname, "ab" );
  endTimer(&end);
  computeTime(&start, &end);


	if ( !file ){
		fprintf(stderr,"unable to open file : %s\n",fname ) ;
	} else {
		fileArray.push_back( file );
		byte_order.push_back( false );
		header_type.push_back( sizeof(int) );
		*fileDescriptor = fileArray.size();
	}

	////////////////////////////////////////////////
	//unsigned long long **header_table;
	SerialFile->offset_table = ( unsigned long long ** )calloc(SerialFile->nfields,
			sizeof(unsigned long long *));

	int j;
	for ( j = 0; j < SerialFile->nfields; j++ )
	{
		SerialFile->offset_table[j]=( unsigned long long * ) calloc( SerialFile->nppf ,
				sizeof( unsigned long long));
	}

	// Read in the offset table ...
	for ( j = 0; j < SerialFile->nfields; j++ )
	{

		memcpy( SerialFile->offset_table[j],
				SerialFile->masterHeader +
				VERSION_INFO_HEADER_SIZE +
				j * SerialFile->nppf * sizeof(unsigned long long),
				SerialFile->nppf * sizeof(unsigned long long) );

		if(diff_endian) {
			SwapArrayByteOrder_(  SerialFile->offset_table[j],
			sizeof(unsigned long long int),
			SerialFile->nppf);
			}
		// Swap byte order if endianess is different ...
		/*if ( PhastaIOActiveFiles[i]->Wrong_Endian )
			{
			SwapArrayByteOrder_( PhastaIOActiveFiles[i]->my_read_table[j],
			sizeof(long long int),
			PhastaIOActiveFiles[i]->nppp );
			}
			*/
	}

	////////////////////////////////////////////////
	delete [] fname;
	delete [] imode;
	//free(fname);
	//free(imode);
	free(buffer);
	free(buffer2);
}

//CHANGE END////////////////////////////////////////////////
void vtkPhastaReader::closefile( int* fileDescriptor,
		const char mode[] )
//CHANGE///////////////////////////////////////////////
{
	char* imode = StringStripper( mode );

	if( cscompare( "write", imode )
			|| cscompare( "append", imode ) ) {
		fflush( fileArray[ *fileDescriptor - 1 ] );
	}

	fclose( fileArray[ *fileDescriptor - 1 ] );
	delete [] imode;
}


//CHANGE END///////////////////////////////////////////


void vtkPhastaReader::readheader( int* fileDescriptor,
		const char keyphrase[],
		void* valueArray,
		int*  nItems,
		const char  datatype[],
		const char  iotype[] )
//CHANGE////////////////////////////////////////////////////
{
	int filePtr = *fileDescriptor - 1;
	FILE* fileObject;
	int* valueListInt;

	if ( *fileDescriptor < 1 || *fileDescriptor > (int)fileArray.size() ) {
		fprintf(stderr,"No file associated with Descriptor %d\n",*fileDescriptor);
		fprintf(stderr,"openfile_ function has to be called before \n") ;
		fprintf(stderr,"acessing the file\n ") ;
		fprintf(stderr,"fatal error: cannot continue, returning out of call\n");
		return;
	}

	LastHeaderKey[ filePtr ] = const_cast< char* >( keyphrase );
	LastHeaderNotFound = false;

	fileObject = fileArray[ filePtr ] ;
	Wrong_Endian = byte_order[ filePtr ];

	isBinary( iotype );
	typeSize( datatype );   //redundant call, just avoid a compiler warning.

	// right now we are making the assumption that we will only write integers
	// on the header line.

	valueListInt = static_cast< int* >( valueArray );

	/////////////////////////////////////////////////////////
	int j;
	bool FOUND = false ;
	unsigned int skip_size;
	char * token;
	char readouttag[MAX_FIELDS_NUMBER][MAX_FIELDS_NAME_LENGTH];

	int string_length = strlen( keyphrase );
	char* buffer = (char*) malloc ( string_length+1 );
	strcpy ( buffer, keyphrase );
	buffer[ string_length ] = '\0';

	char* st2 = strtok ( buffer, "@" );
	st2 = strtok (NULL, "@");
	SerialFile->GPid = atoi(st2);
	if ( char* p = strpbrk(buffer, "@") )
		*p = '\0';

	//printf("field is %s and nfields is %d\n",keyphrase,SerialFile->nfields);

	for ( j = 0; j<SerialFile->nfields; j++ )
	{
		memcpy( readouttag[j],
				SerialFile->masterHeader + j*MAX_FIELDS_NAME_LENGTH+MAX_FIELDS_NAME_LENGTH*2+1,
				MAX_FIELDS_NAME_LENGTH-1 );
	}

	for ( j = 0; j<SerialFile->nfields; j++ )
	{
		token = strtok ( readouttag[j], ":" );

		if ( cscompare( buffer, token ) )
		{
			SerialFile->read_field_count = j;
			FOUND = true;
			break;
		}
	}
	if (!FOUND)
	{
		printf("Not found %s \n",keyphrase);
		return;
	}

	int read_part_count =  SerialFile->GPid - ( SerialFile->fileID - 1 ) * SerialFile->nppf - 1;
	SerialFile->my_offset = SerialFile->offset_table[SerialFile->read_field_count][read_part_count];


	//printf("GP id is %d and fileID is %d and nppf is %d; ",SerialFile->GPid,SerialFile->fileID,SerialFile->nppf);
	//printf("read field count is %d and read part count is %d; ",SerialFile->read_field_count,read_part_count);

	char read_out_header[MAX_FIELDS_NAME_LENGTH];
	fseek(fileObject, SerialFile->my_offset+1, SEEK_SET);
	fread( read_out_header, 1, MAX_FIELDS_NAME_LENGTH-1, fileObject );


	token = strtok ( read_out_header, ":" );

	if( cscompare( keyphrase , token ) )
	{
		FOUND = true ;
		token = strtok( NULL, " ,;<>" );
		skip_size = atoi( token );
		for( j=0; j < *nItems && ( token = strtok( NULL," ,;<>") ); j++ )
			valueListInt[j] = atoi( token );
		//printf("$$Keyphrase is %s Value list [0] is %d \n",keyphrase,valueListInt[0] );
		if ( j < *nItems )
		{
			fprintf( stderr, "Expected # of ints not found for: %s\n", keyphrase );
		}
	}

	/////////////////////////////////////////////////////////

	byte_order[ filePtr ] = Wrong_Endian ;

	//if ( ierr ) LastHeaderNotFound = true;

	free(buffer);

	return;
}

//CHANGE END////////////////////////////////////////////////

void vtkPhastaReader::readdatablock( int*  fileDescriptor,
		const char keyphrase[],
		void* valueArray,
		int*  nItems,
		const char  datatype[],
		const char  iotype[] )
//CHANGE//////////////////////////////////////////////////////
{

	int filePtr = *fileDescriptor - 1;
	FILE* fileObject;
	char junk;

	if ( *fileDescriptor < 1 || *fileDescriptor > (int)fileArray.size() ) {
		fprintf(stderr,"No file associated with Descriptor %d\n",*fileDescriptor);
		fprintf(stderr,"openfile_ function has to be called before \n") ;
		fprintf(stderr,"acessing the file\n ") ;
		fprintf(stderr,"fatal error: cannot continue, returning out of call\n");
		return;
	}

	// error check..
	// since we require that a consistant header always preceed the data block
	// let us check to see that it is actually the case.

	if ( ! cscompare( LastHeaderKey[ filePtr ], keyphrase ) ) {
		fprintf(stderr, "Header not consistant with data block\n");
		fprintf(stderr, "Header: %s\n", LastHeaderKey[ filePtr ] );
		fprintf(stderr, "DataBlock: %s\n ", keyphrase );
		fprintf(stderr, "Please recheck read sequence \n");
		if( Strict_Error ) {
			fprintf(stderr, "fatal error: cannot continue, returning out of call\n");
			return;
		}
	}

	if ( LastHeaderNotFound ) return;

	fileObject = fileArray[ filePtr ];
	Wrong_Endian = byte_order[ filePtr ];
	//printf("in readdatablock(): wrong_endian = %d\n", Wrong_Endian);

	size_t type_size = typeSize( datatype );
	int nUnits = *nItems;
	isBinary( iotype );

	if ( binary_format ) {
		fseek(fileObject, SerialFile->my_offset+DB_HEADER_SIZE, SEEK_SET);

		fread( valueArray, type_size, nUnits, fileObject );
		//fread( &junk, sizeof(char), 1 , fileObject );
		//if ( Wrong_Endian ) SwapArrayByteOrder_( valueArray, type_size, nUnits );
		if ( diff_endian )
      SwapArrayByteOrder_( valueArray, type_size, nUnits ); // fj
	} else {

		char* ts1 = StringStripper( datatype );
		if ( cscompare( "integer", ts1 ) ) {
			for( int n=0; n < nUnits ; n++ )
				fscanf(fileObject, "%d\n",(int*)((int*)valueArray+n) );
		} else if ( cscompare( "double", ts1 ) ) {
			for( int n=0; n < nUnits ; n++ )
				fscanf(fileObject, "%lf\n",(double*)((double*)valueArray+n) );
		}
		delete [] ts1;
	}
	return;
}

// End of copy from phastaIO


vtkPhastaReader::vtkPhastaReader()
{
  //this->DebugOn(); // TODO: comment out this line to turn off debug
	this->GeometryFileName = NULL;
	this->FieldFileName = NULL;
	this->SetNumberOfInputPorts(0);
	this->Internal = new vtkPhastaReaderInternal;

	//////////
	this->Parser = 0;
	this->FileName = 0;
	//////////

}

vtkPhastaReader::~vtkPhastaReader()
{
	if (this->GeometryFileName)
	{
		delete [] this->GeometryFileName;
	}
	if (this->FieldFileName)
	{
		delete [] this->FieldFileName;
	}
	delete this->Internal;

	////////////////////
	if (this->Parser)
		this->Parser->Delete();
	////////////////////

}

void vtkPhastaReader::ClearFieldInfo()
{
	this->Internal->FieldInfoMap.clear();
}

void vtkPhastaReader::SetFieldInfo(const char* paraviewFieldTag,
		const char* phastaFieldTag,
		int index,
		int numOfComps,
		int dataDependency,
		const char* dataType)
{
	//printf("In P setfino\n");

	//CHANGE/////////////////////////
	partID_counter=0;

	//CHANGE END/////////////////////

	vtkPhastaReaderInternal::FieldInfo &info =
		this->Internal->FieldInfoMap[paraviewFieldTag];

	info.PhastaFieldTag = phastaFieldTag;
	info.StartIndexInPhastaArray = index;
	info.NumberOfComponents = numOfComps;
	info.DataDependency = dataDependency;
	info.DataType = dataType;
}

int vtkPhastaReader::RequestData(vtkInformation*,
		vtkInformationVector**,
		vtkInformationVector* outputVector)
{
	vtkDebugMacro("In P RequestData");

	int firstVertexNo = 0;
	int fvn = 0;
	int noOfNodes, noOfCells, noOfDatas;


	// get the data object
	//TODO Just Testing
	vtkSmartPointer<vtkInformation> outInfo =
		outputVector->GetInformationObject(0);

	//change///////  This part not working
	// get the current piece being requested
	int piece =
		outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

	//printf("piece is %d\n",piece);

	//partID_counter++;
	partID_counter=PART_ID;
	//change end////////////////

	vtkUnstructuredGrid *output = vtkUnstructuredGrid::SafeDownCast(
			outInfo->Get(vtkDataObject::DATA_OBJECT()));

	//printf("This geom file is %s\n",this->GeometryFileName);


	int numPieces=NUM_PIECES, numFiles=NUM_FILES, timeStep=TIME_STEP;
	//int numPieces=8, numFiles=4, timeStep=50400;

	int numPiecesPerFile = numPieces/numFiles;
	int fileID;
	fileID = int((partID_counter-1)/numPiecesPerFile)+1;

  vtkDebugMacro(<< "FILE_PATH: " << FILE_PATH);
  // FILE_PATH is set to be the path of .pht file ?
	//sprintf(this->FieldFileName,"%s%s.%d.%d",FILE_PATH,"/restart-dat",timeStep,fileID); // this is Ning's version

  // the file id of fieldfilename need to be changed to file id now
  char* str = this->FieldFileName;
  for (int i = strlen(this->FieldFileName); i >= 0; i--) {
    if(str[i] != '.')
      str[i] = 0;
    else
      break;
  }
  sprintf(str, "%s%d", str, FILE_ID);
  vtkDebugMacro(<<"tweaked FieldFileName="<<this->FieldFileName);
	///////////////////////////////////////////////////////////

	vtkPoints *points;

	output->Allocate(10000, 2100);

	points = vtkPoints::New();

	vtkDebugMacro(<<"Reading Phasta file...");

	if(!this->GeometryFileName || !this->FieldFileName )
	{
		vtkErrorMacro(<<"All input parameters not set.");
		return 0;
	}
	vtkDebugMacro(<< "Updating ensa with ....");
	vtkDebugMacro(<< "Geom File : " << this->GeometryFileName);
	vtkDebugMacro(<< "Field File : " << this->FieldFileName);

	fvn = firstVertexNo;
	this->ReadGeomFile(this->GeometryFileName, firstVertexNo, points, noOfNodes, noOfCells);
	/* set the points over here, this is because vtkUnStructuredGrid
		 only insert points once, next insertion overwrites the previous one */
	// acbauer is not sure why the above comment is about...
	output->SetPoints(points);
	points->Delete();

	if (!this->Internal->FieldInfoMap.size())
	{
		vtkDataSetAttributes* field = output->GetPointData();
		this->ReadFieldFile(this->FieldFileName, fvn, field, noOfNodes);
	}
	else
	{
		this->ReadFieldFile(this->FieldFileName, fvn, output, noOfDatas);
	}

	// if there exists point arrays called coordsX, coordsY and coordsZ,
	// create another array of point data and set the output to use this
	vtkPointData* pointData = output->GetPointData();
	vtkDoubleArray* coordsX = vtkDoubleArray::SafeDownCast(
			pointData->GetArray("coordsX"));
	vtkDoubleArray* coordsY = vtkDoubleArray::SafeDownCast(
			pointData->GetArray("coordsY"));
	vtkDoubleArray* coordsZ = vtkDoubleArray::SafeDownCast(
			pointData->GetArray("coordsZ"));
	if(coordsX && coordsY && coordsZ)
	{
		vtkIdType numPoints = output->GetPoints()->GetNumberOfPoints();
		if(numPoints != coordsX->GetNumberOfTuples() ||
				numPoints != coordsY->GetNumberOfTuples() ||
				numPoints != coordsZ->GetNumberOfTuples() )
		{
			vtkWarningMacro("Wrong number of points for moving mesh.  Using original points.");
			return 0;
		}
		vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
		points->DeepCopy(output->GetPoints());
		for(vtkIdType i=0;i<numPoints;i++)
		{
			points->SetPoint(i, coordsX->GetValue(i), coordsY->GetValue(i),
					coordsZ->GetValue(i));
		}
		output->SetPoints(points);
	}

	//printf("end of RequestData():");
	//printf("counter = %ld\n", counter++);

  vtkDebugMacro("end of P RequestData() # of pieces is "<<numPieces << ", partID_counter = "<< partID_counter);
  vtkDebugMacro("-------> total open time is "<< opentime_total);

	return 1;
}

/* firstVertexNo is useful when reading multiple geom files and coalescing
	 them into one, ReadGeomfile can then be called repeatedly from Execute with
	 firstVertexNo forming consecutive series of vertex numbers */

void vtkPhastaReader::ReadGeomFile(char* geomFileName,
		int &firstVertexNo,
		vtkPoints *points,
		int &num_nodes,
		int &num_cells)
{
	vtkDebugMacro("in P ReadGeomFile(): partID="<<partID_counter);

	/* variables for vtk */
	vtkUnstructuredGrid *output = this->GetOutput();
	double *coordinates;
	vtkIdType *nodes;
	int cell_type;

	//  int num_tpblocks;
	/* variables for the geom data file */
	/* nodal information */
	// int byte_order;
	//int data[11], data1[7];
	int dim;
	int num_int_blocks;
	double *pos;
	//int *nlworkdata;
	/* element information */
	int num_elems,num_vertices,num_per_line;
	int *connectivity = NULL;


	/* misc variables*/
	int i, j,k,item;
	int geomfile;

	//CHANGE///////////////////////////////////////
	int getNfiles, getNPPF;
	queryphmpiio_(geomFileName,&getNfiles,&getNPPF);
	char fieldName[255];


	//CHANGE END///////////////////////////////////

	openfile(geomFileName,"read",&geomfile);
	//geomfile = fopen(GeometryFileName,"rb");

	if(!geomfile)
	{
		vtkErrorMacro(<<"Cannot open file " << geomFileName);
		//return;
	}

	int expect;
	int array[10];
	expect = 1;

	/* read number of nodes */

	///CHANGE/////////////////////////////////////////////////////
	bzero((void*)fieldName,255);
	sprintf(fieldName,"%s@%d","number of nodes",partID_counter);
	///CHANGE END//////////////////////////////////////////////////
	readheader(&geomfile,fieldName,array,&expect,"integer","binary");
	//readheader(&geomfile,"number of nodes",array,&expect,"integer","binary");
  vtkDebugMacro("after readheader(), fieldName=" << fieldName << ", geomfile (file desc) = " << geomfile);
	num_nodes = array[0];

	/* read number of elements */

	///CHANGE/////////////////////////////////////////////////////
	bzero((void*)fieldName,255);
	sprintf(fieldName,"%s@%d","number of interior elements",partID_counter);
	///CHANGE END//////////////////////////////////////////////////
	readheader(&geomfile,fieldName,array,&expect,"integer","binary");

	/*readheader(&geomfile,
		"number of interior elements",
		array,
		&expect,
		"integer",
		"binary");
		*/
	num_elems = array[0];
	num_cells = array[0];

	/* read number of interior */

	///CHANGE/////////////////////////////////////////////////////
	bzero((void*)fieldName,255);
	sprintf(fieldName,"%s@%d","number of interior tpblocks",partID_counter);
	///CHANGE END//////////////////////////////////////////////////
	readheader(&geomfile,fieldName,array,&expect,"integer","binary");


	/*readheader(&geomfile,
		"number of interior tpblocks",
		array,
		&expect,
		"integer",
		"binary");
		*/
	num_int_blocks = array[0];

	vtkDebugMacro ( << "Nodes: " << num_nodes
			<< "Elements: " << num_elems
			<< "tpblocks: " << num_int_blocks );

	/* read coordinates */
	expect = 2;

	///CHANGE/////////////////////////////////////////////////////
	bzero((void*)fieldName,255);
	sprintf(fieldName,"%s@%d","co-ordinates",partID_counter);
	///CHANGE END//////////////////////////////////////////////////
	readheader(&geomfile,fieldName,array,&expect,"double","binary");


	//readheader(&geomfile,"co-ordinates",array,&expect,"double","binary");
	// TEST *******************
	num_nodes=array[0];
	// TEST *******************
	if(num_nodes !=array[0])
	{
		vtkErrorMacro(<<"Ambigous information in geom.data file, number of nodes does not match the co-ordinates size. Nodes: " << num_nodes << " Coordinates: " << array[0]);
		return;
	}
	dim = array[1];


	/* read the coordinates */

	coordinates = new double [dim];
	if(coordinates == NULL)
	{
		vtkErrorMacro(<<"Unable to allocate memory for nodal info");
		return;
	}

	pos = new double [num_nodes*dim];
	if(pos == NULL)
	{
		vtkErrorMacro(<<"Unable to allocate memory for nodal info");
		return;
	}

	item = num_nodes*dim;

	//CHANGE
	//readdatablock(&geomfile,"co-ordinates",pos,&item,"double","binary");
	//CHANGE END
	readdatablock(&geomfile,fieldName,pos,&item,"double","binary");

	for(i=0;i<num_nodes;i++)
	{
		for(j=0;j<dim;j++)
		{
			coordinates[j] = pos[j*num_nodes + i];
		}
		switch(dim)
		{
			case 1:
				points->InsertPoint(i+firstVertexNo,coordinates[0],0,0);
				break;
			case 2:
				points->InsertPoint(i+firstVertexNo,coordinates[0],coordinates[1],0);
				break;
			case 3:
				points->InsertNextPoint(coordinates);
				break;
			default:
				vtkErrorMacro(<<"Unrecognized dimension in "<< geomFileName)
					return;
		}
	}

	/* read the connectivity information */
	expect = 7;

	for(k=0;k<num_int_blocks;k++)
	{

		///CHANGE/////////////////////////////////////////////////////
		bzero((void*)fieldName,255);
		sprintf(fieldName,"%s%d@%d","connectivity interior",k+1,partID_counter);
		///CHANGE END//////////////////////////////////////////////////

		readheader(&geomfile,
				fieldName,
				array,
				&expect,
				"integer",
				"binary");

		/*readheader(&geomfile,
			"connectivity interior",
			array,
			&expect,
			"integer",
			"binary");
			*/

		/* read information about the block*/
		num_elems = array[0];
		num_vertices = array[1];
		num_per_line = array[3];
		connectivity = new int [num_elems*num_per_line];

		if(connectivity == NULL)
		{
			vtkErrorMacro(<<"Unable to allocate memory for connectivity info");
			return;
		}

		item = num_elems*num_per_line;
		/*readdatablock(&geomfile,
			"connectivity interior",
			connectivity,
			&item,
			"integer",
			"binary");
			*/

		readdatablock(&geomfile,
				fieldName,
				connectivity,
				&item,
				"integer",
				"binary");

		/* insert cells */
		for(i=0;i<num_elems;i++)
		{
			nodes = new vtkIdType[num_vertices];

			//connectivity starts from 1 so node[j] will never be -ve
			for(j=0;j<num_vertices;j++)
			{
				nodes[j] = connectivity[i+num_elems*j] + firstVertexNo - 1;
			}

			/* 1 is subtracted from the connectivity info to reflect that in vtk
				 vertex  numbering start from 0 as opposed to 1 in geomfile */

			// find out element type
			switch(num_vertices)
			{
				case 4:
					cell_type = VTK_TETRA;
					break;
				case 5:
					cell_type = VTK_PYRAMID;
					break;
				case 6:
					cell_type = VTK_WEDGE;
					break;
				case 8:
					cell_type = VTK_HEXAHEDRON;

					break;
				default:
					vtkErrorMacro(<<"Unrecognized CELL_TYPE in "<< geomFileName)
						return;
			}

			/* insert the element */
			output->InsertNextCell(cell_type,num_vertices,nodes);
			delete [] nodes;
		}
	}
	// update the firstVertexNo so that next slice/partition can be read
	firstVertexNo = firstVertexNo + num_nodes;

	// clean up
	closefile(&geomfile,"read");
	finalizephmpiio_(&geomfile);
	delete [] coordinates;
	delete [] pos;
	delete [] connectivity;

} // end of ReadGeomFile

void vtkPhastaReader::ReadFieldFile(char* fieldFileName,
		int,
		vtkDataSetAttributes *field,
		int &noOfNodes)
{

	vtkDebugMacro("In P ReadFieldFile (vtkDataSetAttr), readheader etc calls need to be updated..");

	int i, j;
	int item;
	double *data;
	int fieldfile;

	int getNfiles, getNPPF;
	queryphmpiio_(fieldFileName,&getNfiles,&getNPPF);

	openfile(fieldFileName,"read",&fieldfile);
	//fieldfile = fopen(FieldFileName,"rb");

	if(!fieldfile)
	{
		vtkErrorMacro(<<"Cannot open file " << FieldFileName)
			return;
	}
	int array[10], expect;

	/* read the solution */
	vtkDoubleArray* pressure = vtkDoubleArray::New();
	pressure->SetName("pressure");
	vtkDoubleArray* velocity = vtkDoubleArray::New();
	velocity->SetName("velocity");
	velocity->SetNumberOfComponents(3);
	vtkDoubleArray* temperature = vtkDoubleArray::New();
	temperature->SetName("temperature");

	expect = 3;
	readheader(&fieldfile,"solution",array,&expect,"double","binary");
	noOfNodes = array[0];
	this->NumberOfVariables = array[1];

	vtkDoubleArray* sArrays[4];
	for (i=0; i<4; i++)
	{
		sArrays[i] = 0;
	}
	item = noOfNodes*this->NumberOfVariables;
	data = new double[item];
	if(data == NULL)
	{
		vtkErrorMacro(<<"Unable to allocate memory for field info");
		return;
	}

	readdatablock(&fieldfile,"solution",data,&item,"double","binary");

	for (i=5; i<this->NumberOfVariables; i++)
	{
		int idx=i-5;
		sArrays[idx] = vtkDoubleArray::New();
		vtksys_ios::ostringstream aName;
		aName << "s" << idx+1 << ends;
		sArrays[idx]->SetName(aName.str().c_str());
		sArrays[idx]->SetNumberOfTuples(noOfNodes);
	}

	pressure->SetNumberOfTuples(noOfNodes);
	velocity->SetNumberOfTuples(noOfNodes);
	temperature->SetNumberOfTuples(noOfNodes);
	for(i=0;i<noOfNodes;i++)
	{
		pressure->SetTuple1(i, data[i]);
		velocity->SetTuple3(i,
				data[noOfNodes + i],
				data[2*noOfNodes + i],
				data[3*noOfNodes + i]);
		temperature->SetTuple1(i, data[4*noOfNodes + i]);
		for(j=5; j<this->NumberOfVariables; j++)
		{
			sArrays[j-5]->SetTuple1(i, data[j*noOfNodes + i]);
		}
	}

	field->AddArray(pressure);
	field->SetActiveScalars("pressure");
	pressure->Delete();
	field->AddArray(velocity);
	field->SetActiveVectors("velocity");
	velocity->Delete();
	field->AddArray(temperature);
	temperature->Delete();

	for (i=5; i<this->NumberOfVariables; i++)
	{
		int idx=i-5;
		field->AddArray(sArrays[idx]);
		sArrays[idx]->Delete();
	}

	// clean up
	closefile(&fieldfile,"read");
	finalizephmpiio_(&fieldfile);
	delete [] data;

} //closes ReadFieldFile (vtkDataSetAttr)


void vtkPhastaReader::ReadFieldFile(char* fieldFileName,
		int,
		vtkUnstructuredGrid *output,
		int &noOfDatas)
{

  vtkDebugMacro("In P ReadFieldFile (vtkUnstructuredGrid): fieldFileName="<<fieldFileName << ", partID_counter=" << partID_counter);

	int i, j, numOfVars;
	int item;
	int fieldfile;

	//printf("In P readfieldfile 2 and %d\n",partID_counter);
	//printf("read filed file name is %s\n",fieldFileName);

	//CHANGE///////////////////////////////////////
	int getNfiles, getNPPF;
	queryphmpiio_(fieldFileName,&getNfiles,&getNPPF);
	char fieldName[255];

	//CHANGE END///////////////////////////////////

	openfile(fieldFileName,"read",&fieldfile);
	//fieldfile = fopen(FieldFileName,"rb");

	//printf("open handle is %d\n",fieldfile);

	if(!fieldfile)
	{
		vtkErrorMacro(<<"Cannot open file " << FieldFileName)
			//return;
	}
	int array[10], expect;

	int activeScalars = 0, activeTensors = 0;

	vtkPhastaReaderInternal::FieldInfoMapType::iterator it = this->Internal->FieldInfoMap.begin();
	vtkPhastaReaderInternal::FieldInfoMapType::iterator itend = this->Internal->FieldInfoMap.end();
	for(; it!=itend; it++)
	{
		const char* paraviewFieldTag = it->first.c_str();
		const char* phastaFieldTag = it->second.PhastaFieldTag.c_str();
		int index = it->second.StartIndexInPhastaArray;
		int numOfComps = it->second.NumberOfComponents;
		int dataDependency = it->second.DataDependency;
		const char* dataType = it->second.DataType.c_str();


		vtkDataSetAttributes* field;
		if(dataDependency)
			field = output->GetCellData();
		else
			field = output->GetPointData();

		// void *data;
		int dtype;  // (0=double, 1=float)
		vtkDataArray *dataArray;
		/* read the field data */
		if(strcmp(dataType,"double")==0)
		{
			dataArray = vtkDoubleArray::New();
			dtype = 0;
		}
		else if(strcmp(dataType,"float")==0)
		{
			dataArray = vtkFloatArray::New();
			dtype=1;
		}
		else
		{
			vtkErrorMacro("Data type [" << dataType <<"] NOT supported");
			continue;
		}

		dataArray->SetName(paraviewFieldTag);
		dataArray->SetNumberOfComponents(numOfComps);

		expect = 3;

		//printf("version 1: hello 3 in P\n");

		///CHANGE/////////////////////////////////////////////////////
		bzero((void*)fieldName,255);
		sprintf(fieldName,"%s@%d",phastaFieldTag,partID_counter);
		///CHANGE END//////////////////////////////////////////////////
		readheader(&fieldfile,fieldName,array,&expect,dataType,"binary");

		//printf("fieldfile is %d expect is %d and datatype is %s\n",fieldfile,expect,dataType);

		//printf("original fieldname is %s and fieldname is %s\n",phastaFieldTag,fieldName);
		//printf("array 0 is %d and array 1 is %d\n",array[0],array[1]);

		//readheader(&fieldfile,phastaFieldTag,array,&expect,dataType,"binary");
		noOfDatas = array[0];
		this->NumberOfVariables = array[1];
		numOfVars = array[1];
		dataArray->SetNumberOfTuples(noOfDatas);

		//printf("hello 4 in P\n");

		if(index<0 || index>numOfVars-1)
		{
			vtkErrorMacro("index ["<<index<<"] is out of range [num. of vars.:"<<numOfVars<<"] for field [paraview field tag:"<<paraviewFieldTag<<", phasta field tag:"<<phastaFieldTag<<"]");

			dataArray->Delete();
			continue;
		}

		//printf("hello 5 in P\n");

		if(numOfComps<0 || index+numOfComps>numOfVars)
		{
			vtkErrorMacro("index ["<<index<<"] with num. of comps. ["<<numOfComps<<"] is out of range [num. of vars.:"<<numOfVars<<"] for field [paraview field tag:"<<paraviewFieldTag<<", phasta field tag:"<<phastaFieldTag<<"]");

			dataArray->Delete();
			continue;
		}

		//printf("hello 6 in P\n");

		item = numOfVars*noOfDatas;

		//printf("item is %d and dtype is %d and numofvars is %d num of data is %d\n",item,dtype,numOfVars,noOfDatas);

		if (dtype==0)
		{  //data is type double

			double *data;
			data = new double[item];

			if(data == NULL)
			{
				vtkErrorMacro(<<"Unable to allocate memory for field info");

				dataArray->Delete();
				continue;
			}

			//printf("hello 8 in P\n");

			//////////CHANGE/////////////////////
			readdatablock(&fieldfile,fieldName,data,&item,dataType,"binary");
			///////////CHANGE END///////////////

			//readdatablock(&fieldfile,phastaFieldTag,data,&item,dataType,"binary");

			//printf("hello 9 in P\n");

			switch(numOfComps)
			{
				case 1 :
					{
						int offset = index*noOfDatas;

						if(!activeScalars)
							field->SetActiveScalars(paraviewFieldTag);
						else
							activeScalars = 1;
						for(i=0;i<noOfDatas;i++)
						{
							dataArray->SetTuple1(i, data[offset+i]);
						}
					}
					break;
				case 3 :
					{
						int offset[3];
						for(j=0;j<3;j++)
							offset[j] = (index+j)*noOfDatas;

						if(!activeScalars)
							field->SetActiveVectors(paraviewFieldTag);
						else
							activeScalars = 1;
						for(i=0;i<noOfDatas;i++)
						{
							dataArray->SetTuple3(i,
									data[offset[0]+i],
									data[offset[1]+i],
									data[offset[2]+i]);
						}
					}
					break;
				case 9 :
					{
						int offset[9];
						for(j=0;j<9;j++)
							offset[j] = (index+j)*noOfDatas;

						if(!activeTensors)
							field->SetActiveTensors(paraviewFieldTag);
						else
							activeTensors = 1;
						for(i=0;i<noOfDatas;i++)
						{
							dataArray->SetTuple9(i,
									data[offset[0]+i],
									data[offset[1]+i],
									data[offset[2]+i],
									data[offset[3]+i],
									data[offset[4]+i],
									data[offset[5]+i],
									data[offset[6]+i],
									data[offset[7]+i],
									data[offset[8]+i]);
						}
					}
					break;
				default:
					vtkErrorMacro("number of components [" << numOfComps <<"] NOT supported");

					dataArray->Delete();
					delete [] data;
					continue;
			}

			// clean up
			delete [] data;

		}
		else if(dtype==1)
		{  // data is type float

			float *data;
			data = new float[item];

			if(data == NULL)
			{
				vtkErrorMacro(<<"Unable to allocate memory for field info");

				dataArray->Delete();
				continue;
			}

			//////////CHANGE/////////////////////
			readdatablock(&fieldfile,fieldName,data,&item,dataType,"binary");
			//////////CHANGE END/////////////////

			//readdatablock(&fieldfile,phastaFieldTag,data,&item,dataType,"binary");

			switch(numOfComps)
			{
				case 1 :
					{
						int offset = index*noOfDatas;

						if(!activeScalars)
							field->SetActiveScalars(paraviewFieldTag);
						else
							activeScalars = 1;
						for(i=0;i<noOfDatas;i++)
						{
							//double tmpval = (double) data[offset+i];
							//dataArray->SetTuple1(i, tmpval);
							dataArray->SetTuple1(i, data[offset+i]);
						}
					}
					break;
				case 3 :
					{
						int offset[3];
						for(j=0;j<3;j++)
							offset[j] = (index+j)*noOfDatas;

						if(!activeScalars)
							field->SetActiveVectors(paraviewFieldTag);
						else
							activeScalars = 1;
						for(i=0;i<noOfDatas;i++)
						{
							//double tmpval[3];
							//for(j=0;j<3;j++)
							//   tmpval[j] = (double) data[offset[j]+i];
							//dataArray->SetTuple3(i,
							//                    tmpval[0], tmpval[1], tmpval[3]);
							dataArray->SetTuple3(i,
									data[offset[0]+i],
									data[offset[1]+i],
									data[offset[2]+i]);
						}
					}
					break;
				case 9 :
					{
						int offset[9];
						for(j=0;j<9;j++)
							offset[j] = (index+j)*noOfDatas;

						if(!activeTensors)
							field->SetActiveTensors(paraviewFieldTag);
						else
							activeTensors = 1;
						for(i=0;i<noOfDatas;i++)
						{
							//double tmpval[9];
							//for(j=0;j<9;j++)
							//   tmpval[j] = (double) data[offset[j]+i];
							//dataArray->SetTuple9(i,
							//                     tmpval[0],
							//                     tmpval[1],
							//                     tmpval[2],
							//                     tmpval[3],
							//                     tmpval[4],
							//                     tmpval[5],
							//                     tmpval[6],
							//                     tmpval[7],
							//                     tmpval[8]);
							dataArray->SetTuple9(i,
									data[offset[0]+i],
									data[offset[1]+i],
									data[offset[2]+i],
									data[offset[3]+i],
									data[offset[4]+i],
									data[offset[5]+i],
									data[offset[6]+i],
									data[offset[7]+i],
									data[offset[8]+i]);
						}
					}
					break;
				default:
					vtkErrorMacro("number of components [" << numOfComps <<"] NOT supported");

					dataArray->Delete();
					delete [] data;
					continue;
			}

			// clean up
			delete [] data;

		}
		else
		{
			vtkErrorMacro("Data type [" << dataType <<"] NOT supported");
			continue;
		}


		field->AddArray(dataArray);

		// clean up
		dataArray->Delete();
		//delete [] data;
	}

	// close up
	closefile(&fieldfile,"read");
	finalizephmpiio_(&fieldfile);

}//closes ReadFieldFile (vtkUnstructuredGrid)

void vtkPhastaReader::PrintSelf(ostream& os, vtkIndent indent)
{
	this->Superclass::PrintSelf(os,indent);

	os << indent << "GeometryFileName: "
		<< (this->GeometryFileName?this->GeometryFileName:"(none)")
		<< endl;
	os << indent << "FieldFileName: "
		<< (this->FieldFileName?this->FieldFileName:"(none)")
		<< endl;
}
