package main

import (
	"flag"
	"fmt"
	"time"
)

func main() {
	format := flag.String("f", "", `Date Format
	01/02 03:04:05PM '06 -0700
	Mon Jan _2 15:04:05 2006
	Mon Jan _2 15:04:05 MST 2006
	Mon Jan 02 15:04:05 -0700 2006
	02 Jan 06 15:04 MST
	02 Jan 06 15:04 -0700
	Monday, 02-Jan-06 15:04:05 MST
	Mon, 02 Jan 2006 15:04:05 MST
	Mon, 02 Jan 2006 15:04:05 -0700
	2006-01-02T15:04:05Z07:00
	2006-01-02T15:04:05.999999999Z07:00
	3:04PM
	Jan _2 15:04:05
	Jan _2 15:04:05.000
	Jan _2 15:04:05.000000
	Jan _2 15:04:05.000000000
	2006-01-02 15:04:05
	2006-01-02
	15:04:05
	`)
	timestamp := flag.Int64("d", 0, "Date in Unix Epoch")
	flag.Parse()

	date := time.UnixMicro(*timestamp)

	formatted := date.Format(*format)

	fmt.Print(formatted)
}
