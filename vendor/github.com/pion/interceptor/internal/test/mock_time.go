package test

import (
	"sync"
	"time"
)

// MockTime is a helper to replace time.Now() for testing purposes.
type MockTime struct {
	m      sync.RWMutex
	curNow time.Time
}

// SetNow sets the current time.
func (t *MockTime) SetNow(n time.Time) {
	t.m.Lock()
	defer t.m.Unlock()
	t.curNow = n
}

// Now returns the current time.
func (t *MockTime) Now() time.Time {
	t.m.RLock()
	defer t.m.RUnlock()
	return t.curNow
}
