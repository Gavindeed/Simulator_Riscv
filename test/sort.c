#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void sort(int *begin, int *end)
{
	if(begin + 1 >= end) return;
	int *left = begin, *right = end - 1;
	int pivot = *left;
	while(left < right)
	{
		while(left < right && *right > pivot) right--;
		if(left < right)
		{
			*left = *right;
			left++;
		}
		while(left < right && *left < pivot) left++;
		if(left < right)
		{
			*right = *left;
			right--;
		}
	}
	*left = pivot;
	sort(begin, left);
	sort(left+1, end);
}

int main(int argc, char **argv)
{
	int num = 100000;
	//clock_t start, finish;
	int *A = malloc(num*sizeof(int));
	for(int i = 0; i < num; i++)
	{
		//scanf("%d", A+i);
		A[i] = num - i;
	}
	//start = clock();
	sort(A, A+num);
	//finish = clock();
	//double duration = (double)(finish - start) / CLOCKS_PER_SEC;
	//printf("Duration: %lf ms\n", duration*1000);
	free(A);
	return 0;
}
