import {type DB, open} from '@op-engineering/op-sqlite';
import chai from 'chai';
import {afterAll, beforeEach, describe, it} from './MochaRNAdapter';

let expect = chai.expect;

let db: DB;

export function blobTests() {
  describe('Blobs', () => {
    beforeEach(async () => {
      try {
        db = open({
          name: 'blobs',
          encryptionKey: 'test',
        });

        await db.execute('DROP TABLE IF EXISTS BlobTable;');
        await db.execute(
          'CREATE TABLE BlobTable ( id INT PRIMARY KEY, content BLOB) STRICT;',
        );
      } catch (e) {
        console.warn('error on before each', e);
      }
    });

    afterAll(() => {
      db.delete();
    });

    it('ArrayBuffer', async () => {
      const uint8 = new Uint8Array(2);
      uint8[0] = 42;

      await db.execute(`INSERT OR REPLACE INTO BlobTable VALUES (?, ?);`, [
        1,
        uint8.buffer,
      ]);

      const result = await db.execute('SELECT content FROM BlobTable;');

      const finalUint8 = new Uint8Array(result.rows[0]!.content as any);
      expect(finalUint8[0]).to.equal(42);
    });

    it('Uint8Array', async () => {
      const uint8 = new Uint8Array(2);
      uint8[0] = 42;

      await db.execute(`INSERT OR REPLACE INTO BlobTable VALUES (?, ?);`, [
        1,
        uint8,
      ]);

      const result = await db.execute('SELECT content FROM BlobTable');

      const finalUint8 = new Uint8Array(result.rows[0]!.content as any);
      expect(finalUint8[0]).to.equal(42);
    });

    it('Uint16Array', async () => {
      const uint8 = new Uint16Array(2);
      uint8[0] = 42;

      await db.execute(`INSERT OR REPLACE INTO BlobTable VALUES (?, ?);`, [
        1,
        uint8,
      ]);

      const result = await db.execute('SELECT content FROM BlobTable');

      const finalUint8 = new Uint8Array(result.rows[0]!.content as any);
      expect(finalUint8[0]).to.equal(42);
    });

    it('Uint8Array in prepared statement', async () => {
      const uint8 = new Uint8Array(2);
      uint8[0] = 46;

      const statement = db.prepareStatement(
        'INSERT OR REPLACE INTO BlobTable VALUES (?, ?);',
      );
      await statement.bind([1, uint8]);

      await statement.execute();

      const result = await db.execute('SELECT content FROM BlobTable');

      const finalUint8 = new Uint8Array(result.rows[0]!.content as any);
      expect(finalUint8[0]).to.equal(46);
    });

    it('Buffer in prepared statement', async () => {
      const uint8 = new Uint8Array(2);
      uint8[0] = 52;

      const statement = db.prepareStatement(
        'INSERT OR REPLACE INTO BlobTable VALUES (?, ?);',
      );

      await statement.bind([1, uint8.buffer]);

      await statement.execute();

      const result = await db.execute('SELECT content FROM BlobTable');

      const finalUint8 = new Uint8Array(result.rows[0]!.content as any);
      expect(finalUint8[0]).to.equal(52);
    });

    it('Large string compression/decompression', async () => {
      const originalText =
        'Aging is a natural biological process influenced by endogenous and exogenous factors such as genetics, environment, and individual lifestyle. The aging-dependent decline in resting and maximum heart rate is a conserved feature across multiple species, including humans. Such changes in heart rhythm control underscore fundamental alterations in the primary cardiac pacemaker, the sinoatrial node (SAN). Older individuals often present symptoms of SAN dysfunction (SND), including sinus bradycardia, sinus arrest, and bradycardia-tachycardia syndrome. These can lead to a broad range of symptoms from palpitations, dizziness to recurrent syncope. The sharp rise in the incidence of SND among individuals over 65 years old, coupled with projected longevity over the next decades, highlights the urgent need for a deeper mechanistic understanding of aging-related SND to develop novel and effective therapeutic alternatives. In this review, we will revisit current knowledge on the ionic and structural remodeling underlying age-related decline in SAN function, and a particular emphasis will be made on new directions for future research.';

      const compressedResult = await db.execute(
        'SELECT zstd_compress(?) as compressed',
        [originalText],
      );

      const compressedBlob = compressedResult.rows![0]!.compressed as ArrayBuffer;

      await db.execute('INSERT INTO BlobTable (id, content) VALUES (?, ?)', [
        2,
        compressedBlob,
      ]);

      const retrieved = await db.execute(
        'SELECT content FROM BlobTable WHERE id = 2',
      );

      const retrievedBlob = retrieved.rows![0]!.content as ArrayBuffer;

      const decompressedResult = await db.execute(
        'SELECT zstd_decompress(?, 1) as decompressed',
        [retrievedBlob],
      );

      const decompressedText = decompressedResult.rows![0]!.decompressed as string;

      expect(decompressedText).to.equal(originalText);
    });
  });
}
