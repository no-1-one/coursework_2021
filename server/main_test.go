package main

import (
	"net/http"
	"net/http/httptest"
	"testing"
)

// TestUniversalPageHandler returns nil
func TestUniversalPageHandler(t *testing.T) {
	req, err := http.NewRequest("GET", "/", nil)
	if err != nil {
		t.Errorf("ERR: %v", err)
	}
	rr := httptest.NewRecorder()

	http.HandlerFunc(UniversalPageHandler("../html/index.html")).ServeHTTP(rr, req)

	if status := rr.Code; status != http.StatusOK {
		t.Errorf("Status code not %d . Got %d instead.", http.StatusOK, status)
	}

}
