package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"net"
	"os"
	"strings"
	"time"

	"github.com/abiosoft/ishell"
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

type readResult struct {
	Msg   string
	Error error
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

func handleInput(c *ishell.Context, command string, wchan chan string) {
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

func readFromSocket(c net.Conn, rchan chan readResult) {
	if c == nil {
		return
	}
	defer c.Close()

	for {
		buf := make([]byte, 8912)
		nr, err := c.Read(buf[:])
		res := readResult{}
		if err != nil || nr == 0 || err == io.EOF {
			res.Error = fmt.Errorf("Couldn't read from socket: %s", err)
			rchan <- res
		}

		data := string(buf[0:nr])
		if data == "" {
			return
		}

		res.Msg = data
		rchan <- res
	}
}

func connectToFMD(shell *ishell.Shell, isInteractive bool, rchan chan readResult, wchan chan string) int {
	c, err := net.Dial("unix", fmdSocket)
	if err != nil {
		log.Println("Unable to connect to FvwmMFL: has \"Module FvwmMFL\" been started?")
		log.Fatal("Connection error ", err)
	}

	go writeToSocket(c, wchan)
	go readFromSocket(c, rchan)

	if isInteractive {
		handleInput(nil, strings.Join(os.Args[1:], " "), wchan)
		return 0
	}

	for fromFMD := range rchan {
		if fromFMD.Error != nil {
			red := color.New(color.BgRed).SprintFunc()
			shell.Println(red(fromFMD.Error))
			// Stop the shell here -- we're done, but we must
			// finalise closing the shell outside of this
			// goroutine.
			shell.Stop()
			return 1
		}
		var msg = fromFMD.Msg
		var cp connectionProfileData
		err := json.Unmarshal([]byte(msg), &cp)

		if err == nil || isInteractive {
			vstr := fmt.Sprintf("*FvwmPrompt %s (%s)\n", cp.ConnectionProfile.Version, cp.ConnectionProfile.VersionInfo)
			shell.Println(vstr)

			cyan := color.New(color.FgCyan).SprintFunc()
			shell.Println(cyan("Press ^D or type 'exit' to end this session\n"))

			red := color.New(color.FgRed).SprintFunc()
			shell.Actions.SetPrompt(red(*cmdLineArgs.promptText))
			time.Sleep(100 * time.Millisecond)
			shell.ShowPrompt(true)
			go shell.Run()
		}
	}
	return 0
}

func main() {
	initCmdlineFlags(&cmdLineArgs)
	writeToFMD := make(chan string)
	readFromFMD := make(chan readResult)

	shell := ishell.New()
	shell.ShowPrompt(false)
	shell.IgnoreCase(false)

	shell.DeleteCmd("help")
	shell.DeleteCmd("clear")

	consoleHistory := os.Getenv("FVWM_USERDIR") + "/" + ".FvwmConsole-History"
	shell.SetHistoryPath(consoleHistory)

	shell.NotFound(func(c *ishell.Context) {
		toSend := strings.Join(c.RawArgs, " ")
		// Quit in fvwm is a special command but it can often lead to
		// surprising results.  Rather than blindly exit, invoke
		// FvwmScript to at least confirm.
		if strings.ToLower(toSend) == "quit" {
			handleInput(c, "Module FvwmScript FvwmScript-ConfirmQuit", writeToFMD)
		} else {
			handleInput(c, toSend, writeToFMD)
		}
	})
	shell.EOF(func(c *ishell.Context) {
		shell.Println("EOF")
		shell.Close()
		os.Exit(0)
	})

	isInteractive = len(os.Args) > 1 && os.Args[1] != "-p"

	exit_code := connectToFMD(shell, isInteractive, readFromFMD, writeToFMD)
	shell.Close()
	os.Exit(exit_code)
}
