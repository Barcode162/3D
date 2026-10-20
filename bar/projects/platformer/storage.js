// storage.js - IndexedDBを使ったシンプルなキー・バリュー保存ライブラリ
//
// localStorageとの違い:
//   - 非同期なのでUIをブロックしない
//   - 保存容量が大きい(localStorageは約5MBまで)
//   - 数値・配列・オブジェクトをそのまま保存できる(文字列変換が不要)
// このアプリのプレビューでも、Safari/Chromeなどの普通のブラウザでもそのまま動く。
//
// 使い方(すべてPromiseを返すので await を付ける):
//   await Storage.set("key", 値);   // 保存
//   await Storage.get("key");        // 取得(なければ undefined)
//   await Storage.remove("key");     // 削除
//   await Storage.keys();            // 全キー一覧
//   await Storage.clear();           // 全削除

const Storage = (() => {
  const DB_NAME = "app-storage";
  const STORE = "kv";

  function openDB() {
    return new Promise((resolve, reject) => {
      const req = indexedDB.open(DB_NAME, 1);
      req.onupgradeneeded = () => req.result.createObjectStore(STORE);
      req.onsuccess = () => resolve(req.result);
      req.onerror = () => reject(req.error);
    });
  }

  async function withStore(mode, action) {
    const db = await openDB();
    return new Promise((resolve, reject) => {
      const tx = db.transaction(STORE, mode);
      const req = action(tx.objectStore(STORE));
      tx.oncomplete = () => {
        db.close();
        resolve(req ? req.result : undefined);
      };
      tx.onerror = () => {
        db.close();
        reject(tx.error);
      };
    });
  }

  return {
    set: (key, value) => withStore("readwrite", (st) => st.put(value, key)),
    get: (key) => withStore("readonly", (st) => st.get(key)),
    remove: (key) => withStore("readwrite", (st) => st.delete(key)),
    keys: () => withStore("readonly", (st) => st.getAllKeys()),
    clear: () => withStore("readwrite", (st) => st.clear()),
  };
})();