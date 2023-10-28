#!/usr/bin/env python3
# coding: utf-8
import json
from pathlib import Path
from random import randint, shuffle

from PIL import Image

total = 0
correct = 0
questions = json.load(
    (Path(__file__).parent / "questions.json").open(encoding="UTF-8"))
pools = '/'.join(sorted(questions.keys() - {"comment"}))
questions = questions[input(f"Which question pool ({pools})? ")]
shuffle(questions)

for k in questions:
    answer = randint(1, 4)
    choices = k["others"][:answer - 1] + \
        [k["correct"]] + k["others"][answer - 1:]
    imagepath = [Path(__file__).parent / "images" /
                 image for image in k["images"]]
    for image in imagepath:
        Image.open(image).show()
    print(k["number"], k["question"])
    for n, choice in enumerate(choices):
        print(n + 1, choice)
    if int(input("? ").strip()) == answer:
        print("Correct!")
        correct += 1
    else:
        print(f"The correct answer is {answer} {choices[answer - 1]}.")
    total += 1
    print(f"{correct}/{total}, {100 * correct / total:.2f}%\n")
