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

const (
	fmdSocket = "/tmp/fvwm_mfl.sock"
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

func readFromSocket(c net.Conn, rchan chan string) {
	if c == nil {
		return
	}
	defer c.Close()

	for {
		buf := make([]byte, 8912)
		nr, err := c.Read(buf[:])
		if err != nil || nr == 0 || err == io.EOF {
			log.Printf("Error reading from UDS: %s\n", err)
			os.Exit(0)
		}

		data := string(buf[0:nr])
		if data == "" {
			return
		}

		rchan <- data
	}
}

func connectToFMD(shell *ishell.Shell, rchan chan string, wchan chan string) {
	c, err := net.Dial("unix", fmdSocket)
	if err != nil {
		log.Fatal("Connection error ", err)
	}

	go writeToSocket(c, wchan)
	go readFromSocket(c, rchan)

	for {
		select {
		case fromFMD := <-rchan:
			var cp connectionProfileData
			err := json.Unmarshal([]byte(fromFMD), &cp)

			if err == nil || isInteractive {
				vstr := fmt.Sprintf("*FvwmPrompt %s (%s)\n", cp.ConnectionProfile.Version, cp.ConnectionProfile.VersionInfo)
				shell.Println(vstr)

				cyan := color.New(color.FgCyan).SprintFunc()
				shell.Println(cyan("Press ^D or type 'exit' to end this session\n"))
			}
		}
	}
}

func main() {
	initCmdlineFlags(&cmdLineArgs)
	writeToFMD := make(chan string)
	readFromFMD := make(chan string)

	shell := ishell.New()
	shell.ShowPrompt(false)
	shell.IgnoreCase(true)

	shell.DeleteCmd("help")
	shell.DeleteCmd("clear")

	consoleHistory := os.Getenv("FVWM_USERDIR") + "/" + ".FvwmConsole-History"
	shell.SetHistoryPath(consoleHistory)

	shell.NotFound(func(c *ishell.Context) {
		handleInput(c, strings.Join(c.Args, " "), writeToFMD)
	})

	// register a function for overriding Fvwm3's "Quit" command, to
	// instead run a FvwmForm.
	shell.AddCmd(&ishell.Cmd{
		Name: "quit",
		Help: "Quit Fvwm3",
		Func: func(c *ishell.Context) {
			handleInput(c, "Module FvwmScript FvwmScript-ConfirmQuit", writeToFMD)
		},
	})

	isInteractive = len(os.Args) > 1 && os.Args[1] != "-p"

	go connectToFMD(shell, readFromFMD, writeToFMD)

	if isInteractive {
		shell.Process(os.Args[1:]...)
	} else {
		red := color.New(color.FgRed).SprintFunc()
		shell.Actions.SetPrompt(red(*cmdLineArgs.promptText))
		time.Sleep(100 * time.Millisecond)
		shell.ShowPrompt(true)
		shell.Run()
	}
}
