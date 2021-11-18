const { createTSFN } = require('bindings')('addon');
const remote = require('@electron/remote');

const callback = (id) => { 
    console.log(id); 
    if (id >= 20) {
        remote.getCurrentWindow().reload()
    }
};

createTSFN(callback);