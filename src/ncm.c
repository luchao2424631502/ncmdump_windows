#include <stdio.h>
#include <pthread.h>
#include "dump.h"
#include "thpool.h"

extern char *optarg;

typedef struct
{
	int thread_id;
	char **file_name;
	int left;
	int right;
} ThreadArgs;

void *thread_function(void *arg)
{
	ThreadArgs *args = (ThreadArgs *)arg;
	char **file_name = args->file_name;
	if (!file_name)
	{
		fprintf(stderr, "thread_function file_name error\n");
		return NULL;
	}

	int left = args->left;
	int right = args->right;


	int success_count = 0;
	for (int i = left; i < right; i++)
	{
		int ret = work_convert_windows(file_name[i]);
		if (ret == 0)
			success_count++;
	}

	// printf("thread: (file_namep[0]=%s, left=%d, right=%d, success_count=%d)\n", file_name[0], left, right, success_count);

	return NULL;
}

int main(size_t argc, char *argv[])
{
	/* 解析命令行 */
	// opterr = 0;
	int option, is_dir = 0;
	int thread_nums = 1; // 默认单线程
	char *work_dir = NULL;
	while ((option = getopt(argc, argv, "o:hd:j:")) != -1)
	{
		switch (option)
		{
		case 'h':
			printf("Usage %s [OPTIONS] FILES \n"
				   "Convert CloudMusic ncm files to mp3/flac files\n\n"
				   "Examples:\n"
				   "\tncm2dump test.ncm\n"
				   "\tncm2dump test1.ncm test2.ncm test3.ncm\n"
				   "\tncm2dump -d ./download/\n"
				   "\tncm2dump -j 6 -d ./download/\n\n"
				   "Options:\n"
				   "\t-h                   display HELP and EXIT\n"
				   "\t-j [N]               start N threads to convert\n"
				   "\t-d <Directory>       batch convert ncm in a specified <directory>\n"
				   "\t-o <file>            place out file in <file>\n"
				   "",
				   argv[0]);
			goto END;
			break;

		case 'o':
			// printf("arg is %s\n", optarg);
			break;
		case 'd':
			is_dir = 1;
			work_dir = optarg;
			// printf("arg is %s\n", optarg);
			break;
		case 'j':
			thread_nums = atoi(optarg);
			// printf("arg is %d\n", thread_nums);
			break;
		default: // 选项和参数不规范直接exit
			goto END;
		}
	}

	// 没有输入文件
	if (optind == argc && !is_dir)
	{
		printf("%s: fatal error: no input files. USE %s -h\n",
			   argv[0], argv[0]);
		goto END;
	}

	char **file_queue = &argv[optind];
	size_t file_nums = argc - optind;

	// 创建线程池
	// threadpool pool = thpool_init(thread_nums);

	char **file_name = NULL;
	int count = 0;
	if (!is_dir)
	{
		count = file_nums;
		file_name = malloc(sizeof(char *) * count);
		for (size_t i = 0; i < file_nums; i++)
		{
			// 过滤不合法文件名
			if (0 == strcmp(file_queue[i] + strlen(file_queue[i]) - 4,
							".ncm"))
			{

				file_name[i] = strdup(file_queue[i]);
				// work_convert_windows(file_queue[i]);

				printf("[Single Thread Convert SUCCESS] %s\n", file_queue[i]);
			}
			else
				printf("[INVALID FILE] %s\n", file_queue[i]);
		}
	}
	else
	{
		// 0. 切换到子目录
		if (chdir(work_dir) == -1)
		{
			printf("chdir ERROR\n");
			goto END;
		}

		// printf("debug %s\n", work_dir);
		// 1. 遍历目录, 得到目录下面的所有文件
		DIR *dir = opendir("./");
		if (!dir)
		{
			printf("opendir ERROR\n");
			goto END;
		}

		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL)
		{
			if (strcmp(entry->d_name + strlen(entry->d_name) - 4,
					   ".ncm"))
				continue;
			count++;
		}

		file_name = malloc(sizeof(char *) * count);
		size_t index = 0;

		rewinddir(dir);
		while ((entry = readdir(dir)) != NULL)
		{
			if (strcmp(entry->d_name + strlen(entry->d_name) - 4,
					   ".ncm"))
				continue;
			file_name[index] = strdup(entry->d_name);
			// thpool_add_work(pool, (wrap_func)work_convert, file_name[index]);
			// work_convert_windows(file_name[index]);
			index++;
		}
	}

	pthread_t threads[1024];
	int thread_ids[1024];
	ThreadArgs args[1024];
	int range_off = 0;
	int range_len = count / thread_nums;
	for (int i = 0; i < thread_nums; i++)
	{
		thread_ids[i] = i;
		int left, right;
		if (i < thread_nums-1) {
			left = range_off;
			right = range_off + range_len;
		} else {
			left = range_off;
			right = count;
		}

		args[i].thread_id = i;
		args[i].file_name = file_name;
		args[i].left = left;
		args[i].right = right;

		if (pthread_create(&threads[i], NULL, thread_function, &args[i]) != 0) {
			perror("Failed to create thread");
			return 1;
		}

		range_off += range_len;
	}

	// 等待所有线程完成
	for (int i = 0; i < thread_nums; ++i)
	{
		if (pthread_join(threads[i], NULL) != 0)
		{
			perror("Failed to join thread");
			return 1;
		}
	}

	// 释放内存
	for (size_t i = 0; i < count; i++)
		if (file_name[i] != NULL)
			free(file_name[i]);
END:
	return 0;
}
