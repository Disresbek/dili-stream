import { apiFetch } from './fetch_utils'

function getSearchBucket(name) {
  const bucket = name.substring(0, Math.min(name.length, 2)).toLowerCase().replaceAll(/[^a-z\d]/g, '');
  return bucket || '@';
}

/**
 * Search GameDB for cover candidates matching an application name.
 *
 * @param {string} name Application name to search for.
 * @returns {Promise<object[]>} Matching cover candidates.
 */
export function searchCovers(name) {
  if (!name) {
    return Promise.resolve([]);
  }
  let searchName = name.replaceAll(/\s+/g, '.').toLowerCase();

  // Use raw.githubusercontent.com to avoid CORS issues as we migrate the CNAME
  let dbUrl = "https://raw.githubusercontent.com/LizardByte/GameDB/gh-pages";
  let bucket = getSearchBucket(name);
  return fetch(`${dbUrl}/buckets/${bucket}.json`).then(function (r) {
    if (!r.ok) throw new Error("Failed to search covers");
    return r.json();
  }).then(maps => Promise.all(Object.keys(maps).map(id => {
    let item = maps[id];
    if (item.name.replaceAll(/\s+/g, '.').toLowerCase().startsWith(searchName)) {
      return fetch(`${dbUrl}/games/${id}.json`).then(function (r) {
        return r.json();
      }).catch(() => null);
    }
    return null;
  }).filter(Boolean)))
    .then(results => results
      .filter(item => item && item.cover && item.cover.url)
      .map(game => {
        const thumb = game.cover.url;
        const dotIndex = thumb.lastIndexOf('.');
        const slashIndex = thumb.lastIndexOf('/');
        if (dotIndex < 0 || slashIndex < 0) {
          return null;
        }
        const slug = thumb.substring(slashIndex + 1, dotIndex);
        return {
          name: game.name,
          key: `igdb_${game.id}`,
          url: `https://images.igdb.com/igdb/image/upload/t_cover_big/${slug}.jpg`,
          saveUrl: `https://images.igdb.com/igdb/image/upload/t_cover_big_2x/${slug}.png`,
        }
      }).filter(Boolean));
}

/**
 * Download a cover picture into Dili's cover folder.
 *
 * @param {object} cover A candidate from searchCovers().
 * @returns {Promise<string>} The saved image path, ready for "image-path".
 */
export async function useCover(cover) {
  const r = await apiFetch('./api/covers/upload', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify({ key: cover.key, url: cover.saveUrl }),
  })
  if (!r.ok) throw new Error('Failed to download cover')
  const body = await r.json()
  return body.path
}

