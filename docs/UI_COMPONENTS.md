# UI Components Catalog

CppAdmin uses Tailwind CSS utility classes and Alpine.js for interactivity. Components live in `views/be/default/` as CSP partials.

## Theme Variables

Nine palettes. Set `settings.theme` to one of the names below. `injectTheme()` emits a `<style>` block with CSS custom properties:

| Theme | `--primary` | `--secondary` | `--light` | `--dark` |
|---|---|---|---|---|
| Blue (default) | `#3B82F6` | `#60A5FA` | `#DBEAFE` | `#1E40AF` |
| Green | `#22C55E` | `#4ADE80` | `#DCFCE7` | `#15803D` |
| Purple | `#A855F7` | `#C084FC` | `#F3E8FF` | `#7E22CE` |
| Red | `#EF4444` | `#F87171` | `#FEE2E2` | `#B91C1C` |
| Orange | `#F97316` | `#FB923C` | `#FFEDD5` | `#C2410C` |
| Teal | `#14B8A6` | `#2DD4BF` | `#CCFBF1` | `#0F766E` |
| Pink | `#EC4899` | `#F472B6` | `#FCE7F3` | `#BE185D` |
| Yellow | `#EAB308` | `#FACC15` | `#FEF9C3` | `#A16207` |
| Indigo | `#6366F1` | `#818CF8` | `#E0E7FF` | `#4338CA` |

Usage in CSP:
```html
<button class="bg-[var(--primary)] text-white">Save</button>
<span class="text-[var(--dark)]">Label</span>
```

## Layout Partials

### `layouts/head.csp`

Emits `<head>` with:
- Tailwind CSS CDN
- Alpine.js CDN
- Theme CSS variables (from `$$data["themeCss"].asString()$$`)
- Page title from `$$data["pageTitle"].asString()$$`

### `layouts/sidebar.csp`

Fixed sidebar with:
- App logo/name from settings
- Navigation grouped by module
- Active state via `$$data["activeMenu"].asString()$$`

### `layouts/topbar.csp`

Top navigation bar:
- Breadcrumbs from `$$data["breadcrumbs"]$$` (JSON array of `{label, url}`)
- Current user name + avatar
- Logout button (POST /auth/logout + CSRF token)

### `layouts/foot.csp`

- Closing `</body></html>`
- Flash message display (success/error from session, consumed once)

### `layouts/full-width.csp`

Full-width layout wrapping body between sidebar/topbar and foot.

---

## Form Components

### Text Input

```html
<div class="mb-4">
  <label class="block text-sm font-medium text-gray-700 mb-1">Name</label>
  <input type="text" name="name"
         value="$$h(data["old"]["name"].asString())$$"
         class="w-full border border-gray-300 rounded-md px-3 py-2 text-sm
                focus:outline-none focus:ring-2 focus:ring-[var(--primary)]">
  <%% if (!data["errors"]["name"].asString().empty()) { %%>
  <p class="mt-1 text-xs text-red-600">$$h(data["errors"]["name"].asString())$$</p>
  <%% } %%>
</div>
```

### Select

```html
<select name="status" class="w-full border border-gray-300 rounded-md px-3 py-2 text-sm">
  <option value="Active" <%% if (data["old"]["status"].asString()=="Active") { %%>selected<%% } %%>>Active</option>
  <option value="Inactive" <%% if (data["old"]["status"].asString()=="Inactive") { %%>selected<%% } %%>>Inactive</option>
</select>
```

### Method Override (PUT/DELETE from form)

```html
<form method="POST" action="/access/users/$$h(data["user"]["id"].asString())$$">
  <input type="hidden" name="_method" value="PUT">
  <input type="hidden" name="_csrf" value="$$h(data["csrfToken"].asString())$$">
  <!-- fields -->
</form>
```

For DELETE (body not parsed — use query param):
```html
<form method="POST"
      action="/access/users/$$h(data["user"]["id"].asString())$$?_method=DELETE&_csrf=$$h(data["csrfToken"].asString())$$">
  <button type="submit" onclick="return confirm('Hapus user ini?')">Delete</button>
</form>
```

### CSRF Token

Always include in unsafe-method forms:
```html
<input type="hidden" name="_csrf" value="$$h(data["csrfToken"].asString())$$">
```

---

## Table Component

```html
<div class="overflow-x-auto">
  <table class="min-w-full divide-y divide-gray-200 text-sm">
    <thead class="bg-gray-50">
      <tr>
        <th class="px-4 py-3 text-left font-medium text-gray-500 uppercase tracking-wider">Name</th>
        <th class="px-4 py-3 text-left font-medium text-gray-500 uppercase tracking-wider">Status</th>
        <th class="px-4 py-3"></th>
      </tr>
    </thead>
    <tbody class="bg-white divide-y divide-gray-200">
      <%% for (const auto &row : data["rows"]) { %%>
      <tr class="hover:bg-gray-50">
        <td class="px-4 py-3">$$h(row["name"].asString())$$</td>
        <td class="px-4 py-3">
          <%% if (row["status"].asString() == "Active") { %%>
          <span class="inline-flex px-2 py-1 text-xs font-medium bg-green-100 text-green-800 rounded-full">Active</span>
          <%% } else { %%>
          <span class="inline-flex px-2 py-1 text-xs font-medium bg-gray-100 text-gray-600 rounded-full">Inactive</span>
          <%% } %%>
        </td>
        <td class="px-4 py-3 text-right space-x-2">
          <a href="/access/users/$$h(row["id"].asString())$$" class="text-[var(--primary)] hover:underline">View</a>
          <a href="/access/users/$$h(row["id"].asString())$$/edit" class="text-yellow-600 hover:underline">Edit</a>
        </td>
      </tr>
      <%% } %%>
    </tbody>
  </table>
</div>
```

---

## Pagination Component

```html
<%% if (data["paginate"]["totalPages"].asInt() > 1) { %%>
<nav class="flex items-center justify-between mt-4 text-sm">
  <p class="text-gray-600">
    Total $$data["paginate"]["total"].asString()$$ item
  </p>
  <ul class="flex space-x-1">
    <%% for (const auto &p : data["paginate"]["pageNumbers"]) { %%>
      <%% int pn = p.asInt(); %%>
      <%% if (pn == -1) { %%>
      <li><span class="px-2 py-1">…</span></li>
      <%% } else if (pn == data["paginate"]["page"].asInt()) { %%>
      <li><span class="px-3 py-1 bg-[var(--primary)] text-white rounded">$$pn$$</span></li>
      <%% } else { %%>
      <li><a href="?page=$$pn$$&q=$$h(data["q"].asString())$$" class="px-3 py-1 border rounded hover:bg-gray-50">$$pn$$</a></li>
      <%% } %%>
    <%% } %%>
  </ul>
</nav>
<%% } %%>
```

---

## Alert / Flash Messages

Displayed in `layouts/foot.csp` using session flash:
```html
<%% if (!data["flashSuccess"].asString().empty()) { %%>
<div x-data="{ show: true }" x-show="show"
     class="fixed bottom-4 right-4 z-50 flex items-center gap-2 bg-green-600 text-white px-4 py-3 rounded shadow">
  <span>$$h(data["flashSuccess"].asString())$$</span>
  <button @click="show=false" class="ml-2 text-white/70 hover:text-white">✕</button>
</div>
<%% } %%>
<%% if (!data["flashError"].asString().empty()) { %%>
<div x-data="{ show: true }" x-show="show"
     class="fixed bottom-4 right-4 z-50 flex items-center gap-2 bg-red-600 text-white px-4 py-3 rounded shadow">
  <span>$$h(data["flashError"].asString())$$</span>
  <button @click="show=false" class="ml-2 text-white/70 hover:text-white">✕</button>
</div>
<%% } %%>
```

---

## Theme Switcher

The settings page includes a live theme preview. JavaScript sends `PUT /setting` with the new `theme` value via fetch, then reloads CSS variables without a full page reload.

```html
<div class="grid grid-cols-3 gap-3">
  <%% for (const auto &t : data["themes"]) { %%>
  <button onclick="previewTheme('$$h(t["name"].asString())$$', '$$h(t["primary"].asString())$$')"
          class="border-2 rounded-lg p-3 text-sm font-medium transition"
          id="theme-btn-$$h(t["name"].asString())$$">
    <span class="w-4 h-4 rounded-full inline-block mr-1"
          style="background:$$h(t["primary"].asString())$$"></span>
    $$h(t["name"].asString())$$
  </button>
  <%% } %%>
</div>
```

---

## Components Showcase Page

`GET /components` renders `views/be/default/components/index.csp` — a catalog of every UI primitive above with live examples. It reads no database, only static view data.
