# silentland

Простой приватный медиахаб на статике + Supabase (auth + storage).

## Быстрый старт
1. Создай проект в Supabase.
2. В Storage создай bucket `media` (private).
3. В SQL Editor добавь политики:
```sql
create policy "authenticated read media"
on storage.objects for select
using (auth.role() = 'authenticated' and bucket_id = 'media');

create policy "authenticated insert media"
on storage.objects for insert
with check (auth.role() = 'authenticated' and bucket_id = 'media');

create policy "authenticated delete media"
on storage.objects for delete
using (auth.role() = 'authenticated' and bucket_id = 'media');
```
4. В `index.html` заполни `APP_CONFIG`:
```js
window.APP_CONFIG = {
  supabaseUrl: "https://YOUR_PROJECT.supabase.co",
  supabaseAnonKey: "YOUR_SUPABASE_ANON_KEY",
  bucket: "media"
};
```
5. Деплой на Vercel:
   - Framework Preset: `Other`
   - Build Command: пусто
   - Output Directory: `.`

## Dev режим
- `?dev=1` показывает сайт за заглушкой?
- `?dev=0` возвращает заглушку
