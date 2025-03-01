PRAGMA foreign_keys = ON;

CREATE TABLE users(
  id VARCHAR(50) NOT NULL,
  balance INTEGER NOT NULL,
  last_updated INTEGER NOT NULL,
  PRIMARY KEY(id)
);

CREATE TABLE items(
  id INTEGER NOT NULL PRIMARY KEY,
  name VARCHAR(50) NOT NULL,
  price INTEGER NOT NULL,
  rate INTEGER NOT NULL
);

CREATE TABLE userItems(
  user_id VARCHAR(50) NOT NULL,
  item_id INTEGER NOT NULL,
  quantity INTEGER NOT NULL,
  PRIMARY KEY(user_id, item_id),
  FOREIGN KEY(user_id) REFERENCES users(id),
  FOREIGN KEY(item_id) REFERENCES items(id)
);