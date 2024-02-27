#in: $1		file 		(e.g. foo)
#in: $2		from_dir 	(e.g. build_dir/)
#in: $3		to_dir 		(e.g. install_dir/)

#install: create link from $1 in directory $2 to directory $3

define install

$(eval TARGET += $3$1)
$(eval INSTALL += $3$1)

$3$1 : $2$1
	ln -f $$< $$@
endef
