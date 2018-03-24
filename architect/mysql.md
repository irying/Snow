## 关于索引优化

1.前导模糊查询不能使用索引

```
select * from doc where title like '%XX'
```

2.非前导模糊查询则可以使用索引，上面改成'xx%';



3.直接告诉mysql怎么做，mysql耗费的cpu最少，但是一般不这么写sql。

比如

```
select * from doc where status=1 union all select * from doc where status =2;
```

**可以用in来解决**。



4.范围查询，负向条件查询

4.1**负向条件查询不能使用索引，可以优化为in查询**

4.2范围列可以用到索引，但是范围列后面的列无法用到索引（索引最多一个范围列）



5.在字段上进行计算不能命中索引。

6.联合索引，最左前缀才能命中。

7.强制类型转换会导致全表扫描

8.**利用覆盖索引来进行查询操作，避免回表。**

被查询的列，数据能从索引中取得，而不用通过行定位符 row-locator 再到 row 上获取，即“被查询列要被所建的索引覆盖”，这能够加速查询速度。



**9.如果有 order by、group by 的场景，请注意利用索引的有序性。**

- order by 最后的字段是组合索引的一部分，并且放在索引组合顺序的最后，避免出现 file_sort 的情况，影响查询性能。
- 例如对于语句 where a=? and b=? order by c，可以建立联合索引(a,b,c)。
- 如果索引中有范围查找，那么索引有序性无法利用，如 `WHERE a>10 ORDER BY b;`，索引(a,b)无法排序。



**10.如果明确知道只有一条结果返回，limit 1 能够提高效率。**

比如如下 SQL 语句：

```
select * from user where login_name=?

```

可以优化为：

```
select * from user where login_name=? limit 1
```

**11.SQL 性能优化 explain 中的 type：至少要达到 range 级别，要求是 ref 级别，如果可以是 consts 最好。**

- consts：单表中最多只有一个匹配行（主键或者唯一索引），在优化阶段即可读取到数据。

- ref：使用普通的索引（Normal Index）。

- range：对索引进行范围检索。

- 当 type=all/index 时，索引物理文件全扫，速度非常慢。

  (根据索引来读取数据，**如果索引已包含了查询数据，只需扫描索引树**，否则执行全表扫描和All类似；  )



## 防止sql注入

1.使用预处理语句，**PDO**比mysqli好的地方就在这里

2.写入数据库的数据要进行特殊字符的**转义**

3.查询错误信息不要返回给用户，将**错误记录到日志**



## Sql语句优化

A.先定位到哪条语句执行慢

1.show profiles 查看所有查询的语句执行耗时等情况

2.show profile for queryId

3.show processlist查看哪个连接出问题



B.优化分3步

1，优化查询过程中的数据访问

2.优化长难的查询语句

3.优化特定类型的查询语句



