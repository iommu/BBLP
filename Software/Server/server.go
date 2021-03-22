package main

import (
    "fmt"
	"flag"
	"os"
	"log"
    "net/http"
    "io/ioutil"
    "encoding/json"
)

type baseResult struct {
    StuNum string
}

func main() {

	var questionFile *string

	questionFile = flag.String("q", "questions.json", "location of questions json file")
	flag.Parse()

	if _, err := os.Stat(*questionFile); os.IsNotExist(err) {
		fmt.Println("Error reading questions json file")
		os.Exit(0)
	}

    http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) { 
		http.ServeFile(w, r, *questionFile) 
	})


    http.HandleFunc("/upload", func(w http.ResponseWriter, r *http.Request){
		body, err := ioutil.ReadAll(r.Body)
        if err != nil {
            log.Printf("Error reading body: %v", err)
            http.Error(w, "can't read body", http.StatusBadRequest)
            return
        }
		decoder := json.NewDecoder(body)
		var jsonFormat baseResult
    	err = decoder.Decode(&jsonFormat)
		fileName := jsonFormat.StuNum+".json"
		fmt.Println(fileName)
		err = ioutil.WriteFile(fileName, body, 0777)
    })

    log.Fatal(http.ListenAndServe(":8081", nil))

}