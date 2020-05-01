export default class Queue {
  constructor() {
    this.data = [];
  }

  add(record) {
    this.data.unshift(record);
  };

  remove = function() {
    this.data.pop();
  };

  first = function() {
    return this.data[0];
  };

  last = function() {
    return this.data[this.data.length - 1];
  };

  size = function() {
    return this.data.length;
  };
}