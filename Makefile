units := shape

.PHONY: all $(units)

all: $(units)

$(units):
	@make --directory=$@
