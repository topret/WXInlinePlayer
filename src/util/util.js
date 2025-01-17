/********************************************************
Copyright (c) <2019> <copyright ErosZy>

"Anti 996" License Version 1.0 (Draft)

Permission is hereby granted to any individual or legal entity
obtaining a copy of this licensed work (including the source code,
documentation and/or related items, hereinafter collectively referred
to as the "licensed work"), free of charge, to deal with the licensed
work for any purpose, including without limitation, the rights to use,
reproduce, modify, prepare derivative works of, distribute, publish
and sublicense the licensed work, subject to the following conditions:

1. The individual or the legal entity must conspicuously display,
without modification, this License and the notice on each redistributed
or derivative copy of the Licensed Work.

2. The individual or the legal entity must strictly comply with all
applicable laws, regulations, rules and standards of the jurisdiction
relating to labor and employment where the individual is physically
located or where the individual was born or naturalized; or where the
legal entity is registered or is operating (whichever is stricter). In
case that the jurisdiction has no such laws, regulations, rules and
standards or its laws, regulations, rules and standards are
unenforceable, the individual or the legal entity are required to
comply with Core International Labor Standards.

3. The individual or the legal entity shall not induce, suggest or force
its employee(s), whether full-time or part-time, or its independent
contractor(s), in any methods, to agree in oral or written form, to
directly or indirectly restrict, weaken or relinquish his or her
rights or remedies under such laws, regulations, rules and standards
relating to labor and employment as mentioned above, no matter whether
such written or oral agreements are enforceable under the laws of the
said jurisdiction, nor shall such individual or the legal entity
limit, in any methods, the rights of its employee(s) or independent
contractor(s) from reporting or complaining to the copyright holder or
relevant authorities monitoring the compliance of the license about
its violation(s) of the said license.

THE LICENSED WORK IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN ANY WAY CONNECTION WITH THE
LICENSED WORK OR THE USE OR OTHER DEALINGS IN THE LICENSED WORK.
*********************************************************/

import EventEmitter from 'eventemitter3';
import inherits from 'inherits';

export default {
  isWeChat() {
    return /MicroMessenger/i.test(window.navigator.userAgent);
  },
  workerify(func, methods = []) {   // call from loader.js  , methods= ['read', 'cancel', 'hasData']
    const funcStr = this.getFuncBody(func.toString());  // funcStr = stream.js的文件内容
    function __Worker__(data) {
      EventEmitter.call(this);
      this.id = 0;
      this.resolves = [];

      const blob = new Blob([funcStr], { type: 'text/javascript' });
      this.url = URL.createObjectURL(blob); // funcStr 当做一个文件的url: "blob:http://192.168.1.5:9080/738cc000-1688-4923-aaa1-625c30298114"
      this.worker = new Worker(this.url);
      this.worker.onmessage = message => {
        const { id, data, destroy, type } = message.data;
        if (destroy) {
          this.resolves = [];
          URL.revokeObjectURL(this.url);
          this.worker.terminate();
          this.worker = null;
        } else if (type == 'event') {
          this.emit(data.type, data.data);
        } else {
          for (let i = this.resolves.length - 1; i >= 0; i--) {
            if (id == this.resolves[i].id) {
              this.resolves[i].resolve(data);
              this.resolves.splice(i, 1);
              break;
            }
          }
        }
      };

      this.worker.postMessage({ type: 'constructor', id: this.id++, data }); // data.url=http://192.168.1.5:8086/live/livestream.flv  -->stream.js
    }

    inherits(__Worker__, EventEmitter);

    for (let i = 0; i < methods.length; i++) {
      const type = methods[i];
      __Worker__.prototype[type] = function(data) {
        return new Promise((resolve, reject) => {
          const id = this.id++;
          this.resolves.push({ id, resolve, reject });
          if (this.worker) {
            this.worker.postMessage({ type, id, data });
          }
        });
      };
    }

    return __Worker__;
  },
  getFuncBody(funcStr) {
    return funcStr
      .trim()
      .match(/^function\s*\w*\s*\([\w\s,]*\)\s*{([\w\W]*?)}$/)[1];
  }
};
