const { createTSFN } = require('bindings')('addon');

const callback = (id) => { 
    console.log(id); 
    if (id >= 8) {
        process.exit(0)
    }
};

createTSFN(callback);