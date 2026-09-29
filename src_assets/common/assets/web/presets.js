import { apiFetch } from './fetch_utils'

/**
 * Ready-made apps that Dili can add for the user.
 * Commands that need to run on the PC itself (like Steam) get the right prefix,
 * so they work whether Dili runs as a Flatpak, in a distrobox or directly.
 */
export function buildPresets(hostPrefix) {
  const host = hostPrefix ? `${hostPrefix} ` : ''
  return [
    {
      id: 'desktop',
      description: 'Your normal desktop, on a screen that fits your device.',
      app: {
        name: 'Desktop',
        'image-path': 'desktop.png',
      },
    },
    {
      id: 'bigpicture',
      description: 'Opens Steam Big Picture when you connect and closes it again when you are done.',
      app: {
        name: 'Steam Big Picture',
        'image-path': 'steam.png',
        detached: [`setsid ${host}steam steam://open/bigpicture`],
        'prep-cmd': [{ do: '', undo: `setsid ${host}steam steam://close/bigpicture` }],
      },
    },
  ]
}

async function hostPrefix() {
  try {
    const status = await fetch('./api/status').then((r) => r.json())
    return status.host_prefix || ''
  } catch (e) {
    return ''
  }
}

export async function loadPresets() {
  const presets = buildPresets(await hostPrefix())
  // Use the Steam that is really installed (normal or Flatpak Steam)
  try {
    const r = await fetch('./api/launchers').then((x) => x.json())
    const bp = (r.launchers || []).find((l) => l.id === 'steam-big-picture')
    const preset = presets.find((p) => p.id === 'bigpicture')
    if (bp && preset) {
      preset.installed = !!bp.installed
      if (bp.installed) {
        preset.app.detached = [`setsid ${bp.detached}`]
        preset.app['prep-cmd'] = [{ do: '', undo: `setsid ${bp.undo}` }]
      }
    }
  } catch (e) {
    // Keep the default commands
  }
  return presets
}

export async function saveApp(app, index = -1) {
  const body = { detached: [], 'prep-cmd': [], ...app, index }
  const r = await apiFetch('./api/apps', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(body),
  })
  return r.status === 200
}

/**
 * Add every ready-made app that is not in the list yet.
 */
export async function addMissingPresets() {
  const [presets, list] = await Promise.all([
    loadPresets(),
    fetch('./api/apps').then((r) => r.json()),
  ])
  const names = new Set((list.apps || []).map((a) => a.name))
  for (const preset of presets) {
    if (preset.installed !== false && !names.has(preset.app.name)) {
      await saveApp(preset.app)
    }
  }
}
