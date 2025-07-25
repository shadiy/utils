package main

import (
	"flag"
	"fmt"
	"os"

	"github.com/go-toast/toast"
)

func main() {
	title := flag.String("t", "Notification", "Notification title")
	message := flag.String("m", "", "Notification message body")
	appID := flag.String("a", "notify-send", "App ID to display as the source")
	icon := flag.String("i", "", "Path to icon (optional)")
	flag.Parse()

	// If message is not provided via flag, check positional argument
	if *message == "" && flag.NArg() > 0 {
		*message = flag.Arg(0)
	}

	if *message == "" {
		fmt.Fprintln(os.Stderr, "Usage: notify-send.exe -a <app name> -t <title> -m <message>")
		os.Exit(1)
	}

	notification := toast.Notification{
		AppID:   *appID,
		Title:   *title,
		Message: *message,
		Icon:    *icon, // Can be left empty
	}

	err := notification.Push()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to send notification: %v\n", err)
		os.Exit(1)
	}
}

