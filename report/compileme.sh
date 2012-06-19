#!/bin/bash

pdflatex embeddedLamp && pdflatex embeddedLamp && bibtex embeddedLamp  && pdflatex embeddedLamp
