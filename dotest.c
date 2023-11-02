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
#include "pcg.h"
#include "questions.h"

static const struct data_index {
    const struct question_t *pool;
    const size_t pool_size;
    const char *human_readable;
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
/// Display an image with the GTK backend
/// @param filename The filename of the image to display
void display_image(const char *filename) {
    const struct image_info_t *image = in_word_set(filename, strlen(filename));
    if (image == NULL) {
        fprintf(stderr, "Image %s not found\n", filename);
        return;
    }
    const void *image_data = image->content;
    const size_t length = image->length;
    printf("Displaying image %s length %zu\n", filename, image->length);

    GtkWidget* window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_window_set_default_size(GTK_WINDOW(window), 50, 30);
    gtk_window_set_title(GTK_WINDOW(window), filename);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(window), TRUE);

    gdk_pixbuf_loader_write(loader, image_data, length, NULL);
    gdk_pixbuf_loader_close(loader, NULL);
    GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
    GtkWidget *image_widget = gtk_image_new_from_pixbuf(pixbuf);
    gtk_container_add(GTK_CONTAINER(window), image_widget);
    gtk_widget_show_all(window);
    gtk_main();
}
#else
/// Dummy function for non-GTK builds
/// @param filename The filename of the image to display
void display_image(const char *filename) {
    printf("Please open %s in your favourite image viewer.\n", filename);
}
#endif


/// Ask for a single unsigned integer.
/// @param prompt The prompt to display.
/// @param max The maximum value the user can enter.
/// @return The integer.
uint64_t input_u64(const char *prompt, uint64_t max) {
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        char buf[10] = {0};
        fgets(buf, 10, stdin);
        uint64_t result = strtoul(buf, NULL, 10);
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

    return (size_t) input_u64("Choice: ", QUESTIONPOOLS_SIZE);
}

/// Create a shuffled array of indices.
/// @param size The size of the array.
/// @return The array. Needs to be `free`d.
size_t *random_indices(size_t size, pcg32_random_t *rng) {
    size_t *indices = (size_t *) malloc(size * sizeof(size_t));
    for (size_t i = 0; i < size; ++i)
        indices[i] = i;

    // Fisher-Yates shuffle
    for (size_t i = size - 1; i > 0; --i) {
        const uint32_t j = pcg32_boundedrand_r(rng, i + 1);
        const size_t tmp = indices[i];
        indices[i] = indices[j];
        indices[j] = tmp;
    }

    return indices;
}

/// Test a single question.
/// @param pool the question pool.
/// @param index the index of the question in the pool. It must be valid.
/// @return 1 if the user answered correctly, 0 otherwise. 2 if the user wants
/// to quit.
uint16_t test_single(const struct question_t *pool, const size_t index, pcg32_random_t *rng) {
    const struct question_t *question = pool + index;
    size_t incorrect_pos_now;
    const size_t correct_pos = (size_t) pcg32_boundedrand_r(rng, 4);
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
        display_image(filename);
    }
    uint16_t choice = (uint16_t) input_u64("Choice: ", 5);
    if (choice == 0)
        // EOF or some non-numeric input
        return 2;
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
void offer_test(const struct question_t *pool, const size_t pool_size, pcg32_random_t *rng) {
    // Do I care about performance? Not really. Just work with cache misses.
    // Create a shuffled array of indices
    const size_t *indices = random_indices(pool_size, rng);
    uint16_t correct_count = 0, total = 0;
    for (; total < pool_size; ++total) {
        const uint16_t result = test_single(pool, indices[total], rng);
        if (result == 2)
            break;
        correct_count += result;
        printf("Score: %d/%d = %g%%\n", correct_count, total + 1, 100.0 * correct_count / (total + 1));
        putchar('\n');
    }
    free((void *) indices);
    printf("Final score: %d/%d = %g%%\n", correct_count, total, 100.0 * correct_count / total);
}

int main(int argc, char **argv) {
    size_t pool;
    pcg32_random_t rng;
    if (argc == 2)
        pool = strtoul(argv[1], NULL, 10);
    else
        pool = input_qb();
#if (ENABLE_GTK + 0) == 1
    gtk_init(NULL, NULL);
#endif
    pcg32_srandom_r(&rng, input_u64("Random seed: ", 0xFFFF), 0);

    offer_test(QUESTIONPOOLS[pool].pool, QUESTIONPOOLS[pool].pool_size, &rng);
    return 0;
}
