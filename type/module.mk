$(this).requires.lib := \
	lexer/liblexer.a

$(info $(this).CXXFLAGS)

# All files in source directory beginning with 'xtest' are optional targets
# (built with 'make opt'). Here we here default targets (built with 'make').
