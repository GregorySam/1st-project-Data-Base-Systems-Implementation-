#include "heap_file.h"
#include "bf.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int Heap_id=1234321;




HP_ErrorCode HP_Init() {

	return HP_OK;
}

HP_ErrorCode HP_CreateIndex(const char *filename) {

	if(BF_CreateFile(filename)==BF_OK)
	{
		int fd;
		if(BF_OpenFile(filename,&fd)==BF_OK)
		{
				BF_Block *block;
				BF_Block_Init(&block);
			if(BF_AllocateBlock(fd,block)==BF_OK)
			{
				char* data;
				data=BF_Block_GetData(block);
				memcpy(data,&Heap_id,4);
				BF_Block_SetDirty(block);
				if(BF_UnpinBlock(block)!=BF_OK){return HP_ERROR;}

				if(BF_CloseFile(fd)!=BF_OK){return HP_ERROR;}
				BF_Block_Destroy(&block);
				return HP_OK;
			}
			return HP_ERROR;
		}
		return HP_ERROR;
	}
	return HP_ERROR;

}

HP_ErrorCode HP_OpenFile(const char *fileName, int *fileDesc){
	static unsigned int open_files=0;
	int heap_fileid;


	if(BF_OpenFile(fileName,fileDesc)==BF_OK && open_files<=BF_MAX_OPEN_FILES)
	{
			BF_Block *block;
			BF_Block_Init(&block);

			if(BF_GetBlock(*fileDesc,0,block)==BF_OK)
			{
					char* data;

					data=BF_Block_GetData(block);
					memcpy(&heap_fileid,data,4);
					if(heap_fileid==Heap_id)
					{
							open_files++;
							BF_UnpinBlock(block);
							BF_Block_Destroy(&block);
							return HP_OK;
					}
					return HP_ERROR;
			}
			 return HP_ERROR;
	}
	return HP_ERROR;
}

HP_ErrorCode HP_CloseFile(int fileDesc) {
		if(BF_CloseFile(fileDesc)==BF_OK)
		{
				return HP_OK;
		}

		return HP_ERROR;
}


HP_ErrorCode HP_InsertEntry(int fileDesc, Record record)
{
		char* data;
		int records_num,blocks_num,i;

		BF_Block *block;
		BF_Block_Init(&block);


		if(BF_GetBlockCounter(fileDesc,&blocks_num)!=BF_OK){return HP_ERROR;}

		if((blocks_num-1)==0)
		{
				if(BF_AllocateBlock(fileDesc,block)!=HP_OK){return HP_ERROR;}
				data=BF_Block_GetData(block);
				records_num=1;
				memcpy(data,&records_num,4);
				memcpy(&data[4],&record,sizeof(Record));
		}
		else
		{
				if(BF_GetBlock(fileDesc,blocks_num-1,block)!=BF_OK){return HP_ERROR;}
				data=BF_Block_GetData(block);
				memcpy(&records_num,data,4);
				i=records_num*sizeof(Record)+4;
				if(i+sizeof(Record)>BF_BLOCK_SIZE)
				{
						BF_Block *block;
						BF_Block_Init(&block);
						if(BF_AllocateBlock(fileDesc,block)!=BF_OK){return HP_ERROR;}
						data=BF_Block_GetData(block);
						records_num=1;
						memcpy(data,&records_num,4);
						memcpy(&data[4],&record,sizeof(Record));
				}
				else
				{
						memcpy(&data[i],&record,sizeof(Record));
						records_num++;
						memcpy(data,&records_num,4);
				}
		}

		BF_Block_SetDirty(block);
		BF_UnpinBlock(block);
		BF_Block_Destroy(&block);
		return HP_OK;

}



HP_ErrorCode HP_PrintAllEntries(int fileDesc)
{
	int blocks_num,index,records_in_block,j,rowId;


	records_in_block=(BF_BLOCK_SIZE-4)/sizeof(Record);


	if(BF_GetBlockCounter(fileDesc,&blocks_num)!=BF_OK){return HP_ERROR;}

	rowId=1;
	for(int i=0;i<=blocks_num-2;i++)
	{
		for(j=rowId;j<=(records_in_block)*(i+1);j++)
		{
			Record record;

			if(HP_GetEntry(fileDesc,j,&record)==HP_ERROR){return HP_ERROR;}
			printf("%d,\"%s\",\"%s\",\"%s\"\n",
			record.id, record.name, record.surname, record.city);
		}

		rowId=j;

	}

	return HP_OK;
}

HP_ErrorCode HP_GetEntry(int fileDesc, int rowId, Record *record) {
	char* data;
	int block_index,record_index,records_in_block,id;

	BF_Block *block;
	BF_Block_Init(&block);

	records_in_block=(BF_BLOCK_SIZE-4)/sizeof(Record);

	block_index=rowId/records_in_block;

	record_index=rowId%records_in_block;
	if(record_index!=0 )
	{
		if(BF_GetBlock(fileDesc,block_index+1,block)!=BF_OK){return HP_ERROR;}
		data=BF_Block_GetData(block);
		memcpy(record,&data[(record_index-1)*sizeof(Record)+4],sizeof(Record));
	}
	else
	{
		if(block_index==0){block_index++;}
		if(BF_GetBlock(fileDesc,block_index,block)!=BF_OK){return HP_ERROR;}
		data=BF_Block_GetData(block);
		memcpy(record,&data[BF_BLOCK_SIZE-sizeof(Record)],sizeof(Record));

	}

	BF_UnpinBlock(block);
	BF_Block_Destroy(&block);

	return HP_OK;
}
