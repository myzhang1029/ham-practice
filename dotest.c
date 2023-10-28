/// This is a massively-overengineered program for practising HAM licensing
/// exam questions.
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#if (ENABLE_GTK + 0) == 1
#include <gtk/gtk.h>
#endif

#include "images.h"
#include "images_gperf.h"
#include "questions.h"

static const struct data_index {
    struct question_t *pool;
    size_t pool_size;
    char *human_readable;
} QUESTIONPOOLS[] = {
    {ised_basic_en, ised_basic_en_count, "ISED Canada Basic in English"},
    {ised_basic_fr, ised_basic_fr_count, "ISED Canada Basic in French"},
    {ised_advanced_en, ised_advanced_en_count, "ISED Canada Advanced in English"},
    {ised_advanced_fr, ised_advanced_fr_count, "ISED Canada Advanced in French"},
    {crac_a_zh, crac_a_zh_count, "CRAC/MIIT Class A in Chinese"},
    {crac_b_zh, crac_b_zh_count, "CRAC/MIIT Class B in Chinese"},
    {crac_c_zh, crac_c_zh_count, "CRAC/MIIT Class C in Chinese"},
    {fcc_technician_en, fcc_technician_en_count, "FCC Technician in English"},
    {fcc_general_en, fcc_general_en_count, "FCC General in English"},
    {fcc_extra_en, fcc_extra_en_count, "FCC Extra in English"},
};

static const size_t QUESTIONPOOLS_SIZE = 10;


#if (ENABLE_GTK + 0) == 1
void display_image(const void *image_data, const size_t length, const char *title) {
    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
    GdkPixbuf *pixbuf;
    GtkWidget* image;

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_window_set_default_size(GTK_WINDOW(window), 50, 30);
    gtk_window_set_title(GTK_WINDOW(window), title);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(window), TRUE);

    gdk_pixbuf_loader_write(loader, image_data, length, NULL);
    gdk_pixbuf_loader_close(loader, NULL);
    pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
    image = gtk_image_new_from_pixbuf(pixbuf);
    gtk_container_add(GTK_CONTAINER(window), image);
    gtk_widget_show_all(window);
    gtk_main();
}
#endif


/// Ask for a single unsigned integer.
/// @param prompt The prompt to display.
/// @param max The maximum value the user can enter.
/// @return The integer.
uint16_t input_u16(const char *prompt, uint16_t max) {
    while (1) {
        char buf[10];
        uint16_t result;
        printf("%s", prompt);
        fflush(stdout);
        fgets(buf, 10, stdin);
        result = strtoul(buf, NULL, 10);
        if (result < max)
            return result;
        puts("Invalid choice");
    }
}

/// Ask the user for a question pool to test.
/// @return The index of the question pool in `QUESTIONPOOLS`.
size_t input_qb(void) {
    puts("Which question pool? Options:");
    for (size_t i = 0; i < QUESTIONPOOLS_SIZE; ++i)
        printf("%zu: %s\n", i, QUESTIONPOOLS[i].human_readable);

    return input_u16("Choice: ", QUESTIONPOOLS_SIZE);
}

/// Create a shuffled array of indices.
/// @param size The size of the array.
/// @return The array. Needs to be `free`d.
size_t *random_indices(size_t size) {
    size_t *indices = malloc(size * sizeof(size_t));
    for (size_t i = 0; i < size; ++i)
        indices[i] = i;

    // Fisher-Yates shuffle
    for (size_t i = size - 1; i > 0; --i) {
        // XXX: Maybe use a better RNG?
        size_t j = rand() % (i + 1);
        size_t tmp = indices[i];
        indices[i] = indices[j];
        indices[j] = tmp;
    }

    return indices;
}

/// Test a single question.
/// @param pool the question pool.
/// @param index the index of the question in the pool. It must be valid.
/// @return 1 if the user answered correctly, 0 otherwise.
uint32_t test_single(struct question_t *pool, size_t index) {
    size_t correct_pos = rand() % 4, incorrect_pos_now;
    uint16_t choice;
    struct question_t *question = &pool[index];
    printf("%s %s\n", question->number, question->question);
    putchar('\n');
    for (incorrect_pos_now = 0; incorrect_pos_now < correct_pos; ++incorrect_pos_now)
        printf("\t%zu %s\n", incorrect_pos_now + 1, question->incorrect[incorrect_pos_now]);
    printf("\t%zu %s\n", correct_pos + 1, question->correct);
    for (; incorrect_pos_now < 3; ++incorrect_pos_now)
        printf("\t%zu %s\n", incorrect_pos_now + 2, question->incorrect[incorrect_pos_now]);
    putchar('\n');
    for (size_t i = 0; i < question->image_count; ++i) {
        const char *filename = question->images[i];
        const struct image_info_t *image = in_word_set(filename, strlen(filename));
        if (image == NULL) {
            fprintf(stderr, "Image %s not found\n", filename);
            continue;
        }
        printf("Displaying image %zu length %zu\n", i + 1, image->length);
#if (ENABLE_GTK + 0) == 1
        display_image(image->content, image->length, filename);
#endif
    }
    choice = input_u16("Choice: ", 5);
    if (choice == 0)
        choice = 1;
    putchar('\n');
    if (choice == correct_pos + 1)
        puts("Correct!");
    else
        printf("Incorrect. The correct answer was %zu %s\n", correct_pos + 1, question->correct);
    return choice == correct_pos + 1;
}

/// Test the user on a question pool.
/// @param pool The question pool.
/// @param pool_size The size of the question pool.
void offer_test(struct question_t *pool, size_t pool_size) {
    // Do I care about performance? Not really. Just work with cache misses.
    // Create a shuffled array of indices
    size_t *indices = random_indices(pool_size);
    uint32_t correct_count = 0, total = 0;
    for (; total < pool_size; ++total) {
        correct_count += test_single(pool, indices[total]);
        printf("Score: %d/%d = %g%%\n", correct_count, total + 1, 100.0 * correct_count / (total + 1));
        putchar('\n');
    }
}

int main(int argc, char **argv) {
    size_t pool;
    if (argc == 2)
        pool = strtoul(argv[1], NULL, 10);
    else
        pool = input_qb();
#if (ENABLE_GTK + 0) == 1
    gtk_init(NULL, NULL);
#endif
    srand(input_u16("Random seed: ", 0xFFFF));
    offer_test(QUESTIONPOOLS[pool].pool, QUESTIONPOOLS[pool].pool_size);
    return 0;
}
