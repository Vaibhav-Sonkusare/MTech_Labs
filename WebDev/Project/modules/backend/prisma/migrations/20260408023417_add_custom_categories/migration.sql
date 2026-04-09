-- CreateTable
CREATE TABLE "CustomCategory" (
    "id" TEXT NOT NULL PRIMARY KEY,
    "userId" TEXT NOT NULL,
    "domain" TEXT NOT NULL,
    "category" TEXT NOT NULL,
    CONSTRAINT "CustomCategory_userId_fkey" FOREIGN KEY ("userId") REFERENCES "User" ("id") ON DELETE RESTRICT ON UPDATE CASCADE
);

-- CreateIndex
CREATE INDEX "CustomCategory_userId_idx" ON "CustomCategory"("userId");

-- CreateIndex
CREATE UNIQUE INDEX "CustomCategory_userId_domain_key" ON "CustomCategory"("userId", "domain");
