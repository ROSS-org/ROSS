all: ross-rossnet-ug.ps ross-rossnet-ug.pdf

ross-rossnet-ug.ps: ross-rossnet-ug.tex
	latex ross-rossnet-ug.tex
	latex ross-rossnet-ug.tex
	latex ross-rossnet-ug.tex
	dvips -t letter ross-rossnet-ug.dvi -o

ross-rossnet-ug.pdf: ross-rossnet-ug.ps
	ps2pdf ross-rossnet-ug.ps ross-rossnet-ug.pdf

clean:
	rm -f *.aux *.dvi *.log *~
