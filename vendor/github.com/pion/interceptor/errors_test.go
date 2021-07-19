package interceptor

import (
	"errors"
	"testing"
)

func TestMultiError(t *testing.T) {
	rawErrs := []error{
		errors.New("err1"), //nolint
		errors.New("err2"), //nolint
		errors.New("err3"), //nolint
		errors.New("err4"), //nolint
	}
	errs := flattenErrs([]error{
		rawErrs[0],
		nil,
		rawErrs[1],
		flattenErrs([]error{
			rawErrs[2],
		}),
	})
	str := "err1\nerr2\nerr3"

	if errs.Error() != str {
		t.Errorf("String representation doesn't match, expected: %s, got: %s", errs.Error(), str)
	}

	errIs, ok := errs.(multiError)
	if !ok {
		t.Fatal("FlattenErrs returns non-multiError")
	}
	for i := 0; i < 3; i++ {
		if !errIs.Is(rawErrs[i]) {
			t.Errorf("'%+v' should contains '%v'", errs, rawErrs[i])
		}
	}
	if errIs.Is(rawErrs[3]) {
		t.Errorf("'%+v' should not contains '%v'", errs, rawErrs[3])
	}
}
