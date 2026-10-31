#define _POSIX_C_SOURCE 200809L

#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

typedef struct
{
    char* text;
    char* sample;
    size_t start;
    size_t end;
    size_t pattern_len;
    size_t* answer;
    size_t answer_len;
} ThreadArgs;

void* worker(void* Args)
{
    ThreadArgs *data = (ThreadArgs *)Args;

    size_t capacity = data->end - data->start + 1;
    data->answer = malloc(capacity * sizeof(size_t));
    data->answer_len = 0;

    if (data->answer == NULL)
    {
        return NULL;
    }

    for (size_t i = data->start; i <= data->end; i++)
    {
        int flag = 1;

        for (size_t j = 0; j < data->pattern_len; j++)
        {
            if (data->text[i + j] != data->sample[j])
            {
                flag = 0;
                break;
            }
        }

        if (flag)
        {
            data->answer[data->answer_len++] = i;
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        char msg[256];
        int len = snprintf(msg, sizeof(msg),
                           "Использование: %s <количество потоков>\n",
                           argv[0]);
        if (len > 0)
            write(2, msg, (size_t)len < sizeof(msg) ? (size_t)len : sizeof(msg) - 1);
        return 1;
    }

    char *endptr;
    long value = strtol(argv[1], &endptr, 10);
    size_t n = (size_t)value;

    char s[] = "qwerty";
    size_t text_size = 10000000;
    size_t pattern_len = strlen(s);

    char *t = malloc(text_size + 1);

    if (t == NULL)
    {
        write(2, "malloc error\n", 13);
        return 1;
    }

    memset(t, 'a', text_size);
    t[text_size] = '\0';

    for (size_t i = 0; i + pattern_len <= text_size; i += 1000)
    {
        memcpy(t + i, s, pattern_len);
    }

    size_t len_text = strlen(t);
    size_t len_sample = strlen(s);
    size_t K = len_text - len_sample + 1;

    size_t start = 0, end = 0;

    pthread_t *threads = malloc(n * sizeof(*threads));
    ThreadArgs *args = malloc(n * sizeof(*args));

    if (threads == NULL || args == NULL)
    {
        write(2, "malloc error\n", 13);
        free(threads);
        free(args);
        return 1;
    }

    struct timespec start_time, end_time;

    // Начинаем измерение
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    for (size_t i = 0; i < n; i++)
    {
        start = i * K / n;
        end = (i + 1) * K / n - 1;

        args[i].start = start;
        args[i].end = end;
        args[i].text = t;
        args[i].sample = s;
        args[i].pattern_len = len_sample;

        pthread_create(&threads[i], NULL, worker, &args[i]);
    }

    // Ждём завершения всех потоков
    for (size_t i = 0; i < n; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Заканчиваем измерение
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    double elapsed =
        (end_time.tv_sec - start_time.tv_sec) +
        (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    char buffer[256];

    int len = snprintf(buffer, sizeof(buffer),
                       "Время выполнения: %.9f секунд\n", elapsed);

    if (len > 0)
        write(1, buffer, (size_t)len);

    // for (size_t i = 0; i < n; i++)
    // {
    //  write(1, buffer, (size_t)len);

    //     for (size_t j = 0; j < args[i].answer_len; j++)
    //     {
    //         len = snprintf(buffer, sizeof(buffer),
    //                        "Найдено на позиции %zu\n",
    //                        args[i].answer[j]);

    //         // if (len > 0)
    //         //     write(1, buffer, (size_t)len);
    //     }

    //     free(args[i].answer);
    // }

    free(args);
    free(threads);
    free(t);

    return 0;
}