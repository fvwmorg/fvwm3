if FVWM_BUILD_GOLANG
GOCMD=go
GOBUILD=$(GOCMD) build
GOCLEAN=$(GOCMD) clean
BINARY_NAME=FvwmPrompt

all: build
build:
	$(GOBUILD) -mod=vendor -o $(BINARY_NAME) -v
clean:
	$(GOCLEAN)
	rm -f $(BINARY_NAME)
	rm -f $(BINARY_UNIX)
run:
	$(GOBUILD) -mod=vendor -o $(BINARY_NAME) -v ./...
	./$(BINARY_NAME)

install:
	rm -f $(DESTDIR)$(bindir)/$(BINARY_NAME)
	install $(BINARY_NAME) $(DESTDIR)$(bindir)

uninstall:
	rm -f $(DESTDIR)$(bindir)/$(BINARY_NAME)
endif
