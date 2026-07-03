# CppAdmin REST API Reference

Base path: `/api/v1`

All API responses use `Content-Type: application/json`.

## Authentication

POST endpoints that mutate state require a valid JWT in the `Authorization: Bearer <token>` header.

### Login

```
POST /api/v1/auth/login
Content-Type: application/json

{ "email": "admin@admin.com", "password": "12345678" }
```

Response `200`:
```json
{
  "success": true,
  "token": "<JWT>",
  "user": { "id": "...", "name": "Admin", "email": "admin@admin.com" }
}
```

Response `401`:
```json
{ "success": false, "message": "Email atau password salah.", "status": 401 }
```

### Register

```
POST /api/v1/auth/register
Content-Type: application/json

{ "name": "...", "email": "...", "password": "...", "phone": "..." }
```

Response `201`: `{ "success": true, "user": { ... } }`

### Forgot Password (request OTP)

```
POST /api/v1/auth/forgot-password
Content-Type: application/json

{ "email": "..." }
```

Response `200`: `{ "success": true, "message": "OTP dikirim via email." }`

Always succeeds (silent if email not found — prevents enumeration).

### Reset Password

```
POST /api/v1/auth/reset-password
Content-Type: application/json

{ "email": "...", "otp": "123456", "newPassword": "newpass123" }
```

Response `200`: `{ "success": true }`

Response `422`: `{ "success": false, "message": "OTP tidak valid.", "status": 422 }`

---

## Users (access.users)

All require `Authorization: Bearer <token>`.

### List Users

```
GET /api/v1/access/users?page=1&pageSize=10&q=search
```

Response `200`:
```json
{
  "success": true,
  "data": [ { "id": "...", "code": "...", "name": "...", "email": "...", "status": "Active" } ],
  "meta": { "total": 42, "page": 1, "pageSize": 10, "totalPages": 5 }
}
```

### Get User

```
GET /api/v1/access/users/:id
```

Response `200`: `{ "success": true, "data": { ... } }`

Response `404`: `{ "success": false, "message": "User tidak ditemukan.", "status": 404 }`

### Create User

```
POST /api/v1/access/users
Content-Type: application/json

{ "name": "...", "code": "USR-001", "email": "...", "password": "...", "status": "Active", "phone": "...", "timezone": "UTC", "roleIds": ["<role-id>"] }
```

Response `201`: `{ "success": true, "data": { ... } }`

### Update User

```
PUT /api/v1/access/users/:id
Content-Type: application/json

{ "name": "...", "status": "...", "phone": "...", "roleIds": ["<role-id>"] }
```

Response `200`: `{ "success": true, "data": { ... } }`

### Delete User

```
DELETE /api/v1/access/users/:id
```

Response `200`: `{ "success": true }`

---

## Roles (access.roles)

### List Roles

```
GET /api/v1/access/roles?page=1&pageSize=10&q=
```

Response `200`: `{ "success": true, "data": [...], "meta": { ... } }`

### Create Role

```
POST /api/v1/access/roles
Content-Type: application/json

{ "name": "Editor", "guard_name": "web", "status": "Active", "desc": "...", "permissionIds": ["..."] }
```

### Update Role

```
PUT /api/v1/access/roles/:id
```

### Delete Role

```
DELETE /api/v1/access/roles/:id
```

---

## Permissions (access.permissions)

### List Permissions

```
GET /api/v1/access/permissions?page=1&pageSize=20&q=&guardName=web
```

### Create Permission

```
POST /api/v1/access/permissions
Content-Type: application/json

{ "name": "products.index", "guard_name": "web", "method": "GET", "status": "Active", "desc": "List products" }
```

---

## Settings

### Get Settings

```
GET /api/v1/setting
```

Response `200`:
```json
{
  "success": true,
  "data": {
    "name": "CppAdmin", "theme": "Blue", "icon": "...",
    "logo": "...", "phone": "...", "address": "...",
    "email": "...", "copyright": "..."
  }
}
```

### Update Settings

```
PUT /api/v1/setting
Content-Type: multipart/form-data  OR  application/json

{ "name": "...", "theme": "Green", "phone": "...", ... }
```

---

## Profile

### Get Current User Profile

```
GET /api/v1/profile
```

### Update Profile

```
PUT /api/v1/profile
Content-Type: application/json

{ "name": "...", "phone": "...", "timezone": "..." }
```

### Change Password

```
PUT /api/v1/profile/change-password
Content-Type: application/json

{ "currentPassword": "...", "newPassword": "..." }
```

---

## Error Response Format

All errors follow:
```json
{
  "success": false,
  "message": "Human-readable error message.",
  "status": 422
}
```

| HTTP Status | AppError class | When |
|---|---|---|
| 400 | `AppError` (default) | Generic bad request |
| 401 | `UnauthorizedError` | Missing/invalid auth |
| 403 | `ForbiddenError` | Insufficient permissions |
| 404 | `NotFoundError` | Resource not found |
| 409 | `ConflictError` | Duplicate email/name |
| 422 | `ValidationError` | Invalid input |
| 500 | (uncaught) | Internal server error |
