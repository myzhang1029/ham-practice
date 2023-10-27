def escape_for_c(s):
    return s.replace("\\", "\\\\").replace("\"", "\\\"")


def gen_one(q, fp):
    print("{", file=fp, end="")
    number = escape_for_c(q["number"])
    print(f".number=\"{number}\",", file=fp, end="")
    question = escape_for_c(q["question"])
    print(f".question=\"{question}\",", file=fp, end="")
    correct = escape_for_c(q["correct"])
    print(f".correct=\"{correct}\",", file=fp, end="")
    print(".incorrect={", file=fp, end="")
    for inc in q["others"][:-1]:
        inc = escape_for_c(inc)
        print(f"\"{inc}\",", file=fp, end="")
    if q["others"]:
        last = escape_for_c(q["others"][-1])
        print(f"\"{last}\"", file=fp, end="")
    print("},", file=fp, end="")
    print(f".incorrect_count={len(q['others'])},", file=fp, end="")
    print(".images={", file=fp, end="")
    for img in q["images"][:-1]:
        img = escape_for_c(img)
        print(f"\"{img}\",", file=fp, end="")
    if q["images"]:
        last = escape_for_c(q["images"][-1])
        print(f"\"{last}\"", file=fp, end="")
    print("},", file=fp, end="")
    print(f".image_count={len(q['images'])}", file=fp, end="")
    print("}", file=fp, end="")


def gen_array(questions, name, fp):
    print(f"static struct question_t {name}[] = {{", file=fp)
    for q in questions[:-1]:
        gen_one(q, fp)
        print(",", file=fp, end="")
    gen_one(questions[-1], fp)
    print("};", file=fp)
    print(f"static const size_t {name}_count = {len(questions)};", file=fp)

def generate_all(questions, fp):
    namemap = {
        "A": "crac_a_zh",
        "B": "crac_b_zh",
        "C": "crac_c_zh",
        "Basic": "ised_basic_en",
        "Advanced": "ised_advanced_en",
        "BasicFR": "ised_basic_fr",
        "AdvancedFR": "ised_advanced_fr",
        "Technician": "fcc_technician_en",
        "General": "fcc_general_en",
        "Extra": "fcc_extra_en",
    }
    print("#include <stddef.h>", file=fp)
    print("""struct question_t {
    /// Question number
    char *number;
    /// Question text
    char *question;
    /// Correct answer
    char *correct;
    /// Incorrect answers
    char *incorrect[5];
    /// Number of incorrect answers
    size_t incorrect_count;
    /// Image file names
    char *images[5];
    /// Number of images
    size_t image_count;
};""", file=fp)
    for name in questions.keys():
        if name in namemap:
            gen_array(questions[name], namemap[name], fp)
        else:
            print(f"Unknown name {name}")
