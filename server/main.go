package main

import (
	"html/template"
	"log"
	"net/http"
)

func main() {
	temp := map[string]string {
		"/": "html/index.html",
		"/igrf": "html/igrf.html",
		"/vsvs": "html/vsvs.html",
		"/contacts": "html/contacts.html",
		"/emulator": "html/emulator.html",
	}
	for key := range temp {
		http.HandleFunc(key, UniversalPageHandler(temp[key]))
	}
	http.Handle("/static/", http.StripPrefix("/static/", http.FileServer(http.Dir("static"))))
	port := ":8080"
	println("Serve listen on port:", port)
	err := http.ListenAndServe(port, nil)
	if err != nil {
		log.Fatal("ListenAndServe", err)
	}
}

// UniversalPageHandler accepts a request to open a page corresponding to an file provided as name
// Returns a handler function, that parses the file and either displays it,
// or returns error code 400, in case of failure
func UniversalPageHandler(name string) func(w http.ResponseWriter, r *http.Request) {
	return func(w http.ResponseWriter, r *http.Request) {
		tmpl, err := template.ParseFiles(name)
		if err != nil {
			http.Error(w, err.Error(), 400)
			return
		}
		if err := tmpl.Execute(w, nil); err != nil {
			http.Error(w, err.Error(), 400)
			return
		}
	}
}
