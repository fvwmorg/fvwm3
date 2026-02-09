package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"net"
	"os"
	"strings"
	"sync"
	"time"

	"github.com/ergochat/readline"
	"github.com/fatih/color"
	"github.com/sirupsen/logrus"
)

var (
	fmdSocket = os.Getenv("FVWMMFL_SOCKET")
)

// getopt parsing
type getOpts struct {
	promptText *string
}

// FvwmPrompt only cares about a connection profile from Fvwm3 and nothing
// else.  Eventually, there should probably be a lightweight API around the
// different JSON objects so that other programs can be written.
type connectionProfileData struct {
	// Nested struct so that we represent the desired JSON:
	//
	// {
	//        "connection_profile": {
	//                version: ...,
	//        },
	// }
	ConnectionProfile struct {
		Version     string `json:"version"`
		VersionInfo string `json:"version_info"`
	} `json:"connection_profile"`
}

var (
	cmdLineArgs   getOpts
	log           = logrus.New()
	isInteractive = false
)

func initCmdlineFlags(cmdline *getOpts) {
	cmdline.promptText = flag.String("p", ">>> ", "The prompt")

	flag.Parse()
}

func handleInput(command string, wchan chan string) {
	if command != "" {
		wchan <- command
	}
}

func writeToSocket(c net.Conn, wchan chan string) {
	for {
		select {
		case fString := <-wchan:
			c.Write([]byte(fString))
		}
	}
}

func readFromSocket(c net.Conn, rchan chan string, rl *readline.Instance) {
	if c == nil {
		return
	}
	defer c.Close()

	for {
		buf := make([]byte, 8912)
		nr, err := c.Read(buf[:])
		if err != nil || nr == 0 || err == io.EOF {
			rl.Close()
		}

		data := string(buf[0:nr])
		if data == "" {
			return
		}

		rchan <- data
	}
}

func connectToFMD(rl *readline.Instance, done chan struct{}, once *sync.Once, rchan chan string, wchan chan string) {
	c, err := net.Dial("unix", fmdSocket)
	if err != nil {
		log.Println("Unable to connect to FvwmMFL: has \"Module FvwmMFL\" been started?")
		log.Fatal("Connection error ", err)
	}

	go writeToSocket(c, wchan)
	go readFromSocket(c, rchan, rl)

	for msg := range rchan {
		var cp connectionProfileData
		err := json.Unmarshal([]byte(msg), &cp)

		if err == nil || isInteractive {
			vstr := fmt.Sprintf("*FvwmPrompt %s (%s)\n", cp.ConnectionProfile.Version, cp.ConnectionProfile.VersionInfo)
			fmt.Fprintln(rl.Stderr(), vstr)

			cyan := color.New(color.FgCyan).SprintFunc()
			fmt.Fprintln(rl.Stderr(), cyan("Press ^D or type 'exit' to end this session\n"))
		}
	}

	// readFromSocket closed the channel; signal the REPL loop to stop.
	once.Do(func() { close(done) })
	rl.Close()
}

func main() {
	initCmdlineFlags(&cmdLineArgs)
	writeToFMD := make(chan string)
	readFromFMD := make(chan string)

	isInteractive = len(os.Args) > 1 && os.Args[1] != "-p"

	consoleHistory := os.Getenv("FVWM_USERDIR") + "/" + ".FvwmConsole-History"

	red := color.New(color.FgRed).SprintFunc()
	rl, err := readline.NewFromConfig(&readline.Config{
		Prompt:      red(*cmdLineArgs.promptText),
		HistoryFile: consoleHistory,
	})
	if err != nil {
		log.Fatal("Failed to initialise readline: ", err)
	}
	defer rl.Close()

	// Signal channel for when FvwmMFL connection dies.
	done := make(chan struct{})
	var once sync.Once

	go connectToFMD(rl, done, &once, readFromFMD, writeToFMD)

	if isInteractive {
		handleInput(strings.Join(os.Args[1:], " "), writeToFMD)
	} else {
		// Give the connection goroutine a moment to print version info.
		time.Sleep(100 * time.Millisecond)

		interruptCount := 0
		for {
			line, err := rl.ReadLine()
			if err == readline.ErrInterrupt {
				interruptCount++
				if interruptCount == 1 {
					fmt.Fprintln(rl.Stderr(), "Input Ctrl-c once more to exit")
				}
				if interruptCount >= 2 {
					break
				}
				continue
			}
			if err != nil { // io.EOF (Ctrl-D) or other error
				fmt.Fprintln(rl.Stderr(), red("EOF"))
				break
			}
			interruptCount = 0

			line = strings.TrimSpace(line)
			if line == "" {
				continue
			}
			if line == "exit" {
				break
			}

			// Quit in fvwm is a special command but it can often lead to
			// surprising results.  Rather than blindly exit, invoke
			// FvwmScript to at least confirm.
			if strings.ToLower(line) == "quit" {
				handleInput("Module FvwmScript FvwmScript-ConfirmQuit", writeToFMD)
			} else {
				handleInput(line, writeToFMD)
			}

			// Check if the connection died while we were processing.
			select {
			case <-done:
				return
			default:
			}
		}
	}
}
