#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct Cache
{
	int valid;
	long tag;
	int lru;
} Cache;

typedef struct Block
{
	Cache **slot;
	int size;
	int hits;
} Block;

void createCaches(Block **hold, int blocks);
void destroyCaches(Block **hold, int blocks);
int countHits(Block **hold, int blocks);
void directMap(Block **hold, long data);
void twoWayMap(Block **hold, long data, int replace);
void fourWayMap(Block **hold, long data, int replace);
void fullMap(Block **hold, long data);

int main(void)
{
	Block *direct[32];
	Block *twoWayR [16], *twoWayLRU[16];
	Block *fourWayR [8], *fourWayLRU[8]; 
	Block *full [1];
	FILE *ifp = fopen("traces.txt", "r");
	char temp[1000];
	long cur;
	int hitsTemp, total = 0;

	srand(time(NULL));

	createCaches(direct, 32);
	createCaches(twoWayR, 16);
	createCaches(twoWayLRU, 16);
	createCaches(fourWayR, 8);
	createCaches(fourWayLRU, 8);
	createCaches(full, 1);
	
	while(!feof(ifp))
	{
		fscanf(ifp, "%s\n", temp);
		cur = strtol(temp, NULL, 0);
		directMap(direct, cur);
		twoWayMap(twoWayR, cur, 1);
		twoWayMap(twoWayLRU, cur, 0);
		fourWayMap(fourWayR, cur, 1);
		fourWayMap(fourWayLRU, cur, 0);
		fullMap(full, cur);
		total++;
	}
	
	hitsTemp = countHits(direct, 32);
	printf("For direct mapping, %d out of %d were hits.\n", hitsTemp, total);
	printf("Hit Rate: %.2f%%\n", ((double) hitsTemp/total)*100);
	printf("\n");
	hitsTemp = countHits(twoWayR, 16);
	printf("For random replacement two-way mapping, %d out of %d were hits.\n", hitsTemp, total);
	printf("Hit Rate: %.2f%%\n", ((double) hitsTemp/total)*100);
	printf("\n");
	hitsTemp = countHits(twoWayLRU, 16);
	printf("For LRU replacement two-way mapping, %d out of %d were hits.\n", hitsTemp, total);
	printf("Hit Rate: %.2f%%\n", ((double) hitsTemp/total)*100);
	printf("\n");
	hitsTemp = countHits(fourWayR, 8);
	printf("For random replacement four-way mapping, %d out of %d were hits.\n", hitsTemp, total);
	printf("Hit Rate: %.2f%%\n", ((double) hitsTemp/total)*100);
	printf("\n");
	hitsTemp = countHits(fourWayLRU, 8);
	printf("For LRU replacement four-way mapping, %d out of %d were hits.\n", hitsTemp, total);
	printf("Hit Rate: %.2f%%\n", ((double) hitsTemp/total)*100);
	printf("\n");
	hitsTemp = countHits(full, 1);
	printf("For random replacement fully associative mapping, %d out of %d were hits.\n", hitsTemp, total);
	printf("Hit Rate: %.2f%%\n", ((double) hitsTemp/total)*100);
	
	destroyCaches(direct, 32);
	destroyCaches(twoWayR, 16);
	destroyCaches(twoWayLRU, 16);
	destroyCaches(fourWayR, 8);
	destroyCaches(fourWayLRU, 8);
	destroyCaches(full, 1);
	fclose(ifp);
	return 0;
}

void createCaches(Block **hold, int blocks)
{
	int i, j, size = (int) 32/blocks;
	
	for(i = 0; i < blocks; i++)
	{
		hold[i] = malloc(sizeof(Block));
		hold[i]->slot = malloc(sizeof(Cache*)*size);
		hold[i]->size = size;
		hold[i]->hits = 0;
		
		for(j = 0; j < size; j++)
		{
			hold[i]->slot[j] = malloc(sizeof(Cache));
			hold[i]->slot[j]->valid = 0;
			hold[i]->slot[j]->tag = 0;
			hold[i]->slot[j]->lru = 0;
		}
	}
}

void destroyCaches(Block **hold, int blocks)
{
	int i, j, size = (int) 32/blocks;
	
	for(i = 0; i < blocks; i++)
	{
		for(j = 0; j < size; j++)
			free(hold[i]->slot[j]);
		
		free(hold[i]->slot);
		free(hold[i]);
	}
}

int countHits(Block **hold, int blocks)
{
	int i, hits = 0;
	
	for(i = 0; i < blocks; i++)
	{
		hits += hold[i]->hits;
	}
	
	return hits;
}

// direct cache has no choice for replacement policy
void directMap(Block **hold, long data)
{
	int index = data%32;
	long tag = data-index;
	
	if(hold[index]->slot[0]->valid == 0 || hold[index]->slot[0]->tag != tag)
	{
		hold[index]->slot[0]->valid = 1;
		hold[index]->slot[0]->tag = tag;
	}
	else
	{
		hold[index]->hits++;
	}
}

// two way can use either LRU or random replacement
void twoWayMap(Block **hold, long data, int replace)
{
	int i, index = data%16, hit = 0, inIndex = -1, max = -1, maxInd = -1;
	long tag = data-index;
	
	for(i = 0; i < 2; i++)
	{
		if(hold[index]->slot[i]->valid == 0 && inIndex == -1)
		{
			inIndex = i;
		}
		else if(hold[index]->slot[i]->tag == tag)
		{
			inIndex = i;
			hit = 1;
		}
		
		hold[index]->slot[i]->lru++;
	}
	
	if(replace)
	{
		if(hit == 0 && inIndex != -1)
		{
			hold[index]->slot[inIndex]->valid = 1;
		}
		else if(hit == 0)
		{
			inIndex = rand()%2;
		}
	}
	else
	{
		if(hit == 0 && inIndex != -1)
		{
			hold[index]->slot[inIndex]->valid = 1;
		}
		else if(!hit)
		{
			for(i = 0; i < 2; i++)
			{
				if(hold[index]->slot[i]->lru > max)
				{
					max = hold[index]->slot[i]->lru;
					maxInd = i;
				}
			}
			
			inIndex = maxInd;
		}
		
		hold[index]->slot[inIndex]->lru = 0;
	}
	
	hold[index]->slot[inIndex]->tag = tag;	
	hold[index]->hits += hit;
}

// four way can use either LRU or random replacement
void fourWayMap(Block **hold, long data, int replace)
{
	int i, index = data%8, hit = 0, inIndex = -1, max = -1, maxInd = -1;
	long tag = data-index;
	
	for(i = 0; i < 4; i++)
	{
		if(hold[index]->slot[i]->valid == 0 && inIndex == -1)
		{
			inIndex = i;
		}
		else if(hold[index]->slot[i]->tag == tag)
		{
			inIndex = i;
			hit = 1;
		}
		
		hold[index]->slot[i]->lru++;
	}
	
	if(replace)
	{
		if(hit == 0 && inIndex != -1)
		{
			hold[index]->slot[inIndex]->valid = 1;
		}
		else if(hit == 0)
		{
			inIndex = rand()%4;
		}
	}
	else
	{
		if(hit == 0 && inIndex != -1)
		{
			hold[index]->slot[inIndex]->valid = 1;
		}
		else if(!hit)
		{
			for(i = 0; i < 4; i++)
			{
				if(hold[index]->slot[i]->lru > max)
				{
					max = hold[index]->slot[i]->lru;
					maxInd = i;
				}
			}
			
			inIndex = maxInd;
		}
		
		hold[index]->slot[inIndex]->lru = 0;
	}
	
	hold[index]->slot[inIndex]->tag = tag;
	hold[index]->hits += hit;
}

// LRU replacement is not feasible beyond four way mapping
// therefore associative here uses just random replacement
void fullMap(Block **hold, long data)
{
	int i, index = -1, hit = 0;
	
	for(i = 0; i < 32; i++)
	{
		if(hold[0]->slot[i]->valid == 0 && index == -1)
		{
			index = i;
		}
		else if(hold[0]->slot[i]->tag == data)
		{
			index = i;
			hit = 1;
		}
	}

	if(hit == 0 && index != -1)
	{
		hold[0]->slot[index]->valid = 1;
		hold[0]->slot[index]->tag = data;
	}
	else if(hit == 0)
	{
		index = rand()%32;
		hold[0]->slot[index]->tag = data;
	}
	
	hold[0]->hits += hit;
}