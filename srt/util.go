// The MIT License (MIT)
//
// # Copyright (c) 2022 John
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
package srt

import (
	"bufio"
	"io"

	"github.com/ossrs/go-oryx-lib/aac"
)

type AACReader struct {
	codec aac.ADTS
	r     *bufio.Reader
}

func NewAACReader(f io.Reader) (*AACReader, error) {
	v := &AACReader{}

	var err error
	if v.codec, err = aac.NewADTS(); err != nil {
		return nil, err
	}

	v.r = bufio.NewReaderSize(f, 4096)
	b, err := v.r.Peek(7 + 1024)
	if err != nil {
		return nil, err
	}

	if _, _, err = v.codec.Decode(b); err != nil {
		return nil, err
	}

	return v, nil
}

func (v *AACReader) NextADTSFrame() ([]byte, error) {
	b, err := v.r.Peek(7 + 1024)
	if err != nil {
		return nil, err
	}

	_, left, err := v.codec.Decode(b)
	if err != nil {
		return nil, err
	}

	adts := b[:len(b)-len(left)]
	if _, err = v.r.Discard(len(adts)); err != nil {
		return nil, err
	}

	return adts, nil
}
