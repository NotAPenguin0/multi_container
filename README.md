# multi_container

Container that is capable of storing and iterating over any number of other containers. Another name would be `tied_container`. It also features a `multi_iterator` class that is used for iterating over it, and works much like a `zip_iterator`. One important note is that `multi_container` owns the containers it stores, and copies the containers given to it in the constructor.
